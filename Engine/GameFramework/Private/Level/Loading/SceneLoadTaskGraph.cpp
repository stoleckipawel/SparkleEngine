#include "PCH.h"

#include "Level/Loading/SceneLoadTaskGraph.h"

#include "Assets/Loading/SceneAssetFileReader.h"
#include "Assets/Loading/SceneAssetPayloadDecoder.h"
#include "Core/Public/Diagnostics/Error.h"
#include "Level/Loading/SceneLoadWorkState.h"
#include "Level/Loading/SceneLoadPackageBuilder.h"
#include "Tasks/Public/TaskExecutionContext.h"

#include <format>

class SceneLoadBudget final
{
  public:
	static constexpr std::size_t kMaximumRetainedLoadBytes = 512ull * 1024ull * 1024ull;
	static constexpr std::size_t kDecodedByteWeight = 4;

	static void ReserveBytes(std::atomic<std::size_t>& bytes, std::size_t amount)
	{
		if (amount > kMaximumRetainedLoadBytes)
			throw Diagnostics::Error("Scene load exceeded its retained-data byte budget.");
		std::size_t current = bytes.load(std::memory_order_relaxed);
		while (current <= kMaximumRetainedLoadBytes - amount &&
		       !bytes.compare_exchange_weak(
		           current, current + amount, std::memory_order_acq_rel, std::memory_order_relaxed))
		{
		}
		if (current > kMaximumRetainedLoadBytes - amount)
			throw Diagnostics::Error("Scene load exceeded its retained-data byte budget.");
	}
};

namespace Assets
{
	CompiledTaskGraph BuildSceneLoadTaskGraph(const std::shared_ptr<SceneLoadWorkState>& state)
	{
		TaskGraphBuilder graph(TaskGraphLimits{
		    .MaximumTasks = static_cast<std::uint32_t>(state->Assets.size() * 2u + 2u),
		    .MaximumEdges = static_cast<std::uint32_t>(state->Assets.size() * 2u + 1u)});
		std::vector<TaskNodeHandle> decodedNodes;
		decodedNodes.reserve(state->Assets.size());
		for (std::size_t index = 0; index < state->Assets.size(); ++index)
		{
			const TaskNodeHandle read = graph.Add(
			    TaskDesc{TaskName(std::format("Read scene asset {}", index)), TaskLane::BlockingIo},
			    [state, index](TaskExecutionContext& context)
			    {
				    if (context.IsCancellationRequested())
					    return TaskResult::Cancelled("Scene load cancelled before asset read.");
				    SceneAssetLoadWork& work = state->Assets[index];
				    SceneAssetFileReader::Read(
				        work.Id,
				        work.ManifestPath,
				        work.Manifest,
				        work.Files,
				        state->RetainedBytes,
				        SceneLoadBudget::kMaximumRetainedLoadBytes);
				    state->Stage.store(LevelLoadOperationStage::Decoding, std::memory_order_release);
				    return TaskResult::Success();
			    });
			const TaskNodeHandle decode = graph.Add(
			    TaskDesc{TaskName(std::format("Decode scene asset {}", index)), TaskLane::Background},
			    [state, index](TaskExecutionContext& context)
			    {
				    if (context.IsCancellationRequested())
					    return TaskResult::Cancelled("Scene load cancelled before asset decode.");
				    SceneAssetLoadWork& work = state->Assets[index];
				    if (work.Files.GetByteCount() > SceneLoadBudget::kMaximumRetainedLoadBytes / SceneLoadBudget::kDecodedByteWeight)
					    throw Diagnostics::Error("Scene load exceeded its weighted decode byte budget.");
				    SceneLoadBudget::ReserveBytes(
				        state->RetainedBytes,
				        work.Files.GetByteCount() * SceneLoadBudget::kDecodedByteWeight);
				    work.Payload = SceneAssetPayloadDecoder::Decode(work.Manifest, work.Files);
				    const std::size_t decodedBytes = SceneLoadPackageBuilder::BuildAssetBlueprints(work);
				    const std::size_t reservedDecodedBytes = work.Files.GetByteCount() * SceneLoadBudget::kDecodedByteWeight;
				    if (decodedBytes > reservedDecodedBytes)
					    SceneLoadBudget::ReserveBytes(state->RetainedBytes, decodedBytes - reservedDecodedBytes);
				    state->RetainedBytes.fetch_sub(work.Files.Reset(), std::memory_order_acq_rel);
				    work.Manifest = {};
				    state->CompletedDecodes.fetch_add(1, std::memory_order_relaxed);
				    return TaskResult::Success();
			    });
			graph.DependsOn(decode, read);
			decodedNodes.push_back(decode);
		}

		const TaskNodeHandle validated = graph.WhenAll(
		    TaskDesc{TaskName("Validate scene load package"), TaskLane::Background}, decodedNodes);
		graph.ContinueWith(
		    validated,
		    TaskDesc{TaskName("Publish scene load package"), TaskLane::Background},
		    [state](TaskExecutionContext& context)
		    {
			    if (context.IsCancellationRequested())
				    return TaskResult::Cancelled("Scene load cancelled before package publication.");
			    state->Stage.store(LevelLoadOperationStage::Validating, std::memory_order_release);
			    SceneLoadPackageBuilder::Finalize(*state);
			    state->Stage.store(LevelLoadOperationStage::Ready, std::memory_order_release);
			    return TaskResult::Success();
		    });
		return graph.Compile();
	}
}

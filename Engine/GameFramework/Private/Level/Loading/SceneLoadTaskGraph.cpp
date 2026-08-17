#include "PCH.h"

#include "Level/Loading/SceneLoadTaskGraph.h"

#include "Assets/Loading/SceneAssetFileReader.h"
#include "Assets/Loading/SceneAssetPayloadDecoder.h"
#include "Core/Public/Diagnostics/Error.h"
#include "Level/Loading/SceneLoadBudget.h"
#include "Level/Loading/SceneLoadWorkState.h"
#include "Level/Loading/SceneLoadPackageBuilder.h"
#include "Tasks/Public/TaskExecutionContext.h"

#include <format>

namespace Assets
{
	namespace SceneLoadTaskGraphDetail
	{
		constexpr std::size_t MaximumSceneAssetsInFlight = 2;
		// Decode publishes expanded runtime objects and nested containers while the raw bytes are still retained.
		// Admit a conservative expansion allowance before allocation, then reconcile it against measured capacities.
		constexpr std::size_t DecodedByteReservationWeight = 4;

		void ReserveOrThrow(SceneLoadBudget& budget, std::size_t amount)
		{
			if (!budget.TryReserve(amount))
			{
				throw Diagnostics::Error("Scene load exceeded the aggregate retained-data byte budget.");
			}
		}
	}

	CompiledTaskGraph BuildSceneLoadTaskGraph(const std::shared_ptr<SceneLoadWorkState>& state)
	{
		const std::size_t assetCount = state->Assets.size();
		const std::size_t throttlingEdgeCount = assetCount > SceneLoadTaskGraphDetail::MaximumSceneAssetsInFlight
		    ? assetCount - SceneLoadTaskGraphDetail::MaximumSceneAssetsInFlight
		    : 0;
		TaskGraphBuilder graph(
		    TaskGraphLimits{
		        .MaximumTasks = static_cast<std::uint32_t>(assetCount * 2u + 2u),
		        .MaximumEdges = static_cast<std::uint32_t>(assetCount * 2u + throttlingEdgeCount + 1u)});
		std::vector<TaskNodeHandle> decodedNodes;
		decodedNodes.reserve(assetCount);
		for (std::size_t index = 0; index < state->Assets.size(); ++index)
		{
			const TaskNodeHandle read = graph.Add(
			    TaskDesc{TaskName(std::format("Read scene asset {}", index)), TaskLane::BlockingIo},
			    [state, index](TaskExecutionContext& context)
			    {
				    if (context.IsCancellationRequested())
					    return TaskResult::Cancelled("Scene load cancelled before asset read.");
				    SceneAssetLoadWork& work = state->Assets[index];
				    work.RetainedManifestBytes =
				        SceneAssetFileReader::Read(work.Id, work.ManifestPath, work.Manifest, work.Files, state->Budget);
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
				    const std::size_t rawBytes = work.Files.GetByteCount();
				    if (rawBytes > state->Budget.GetMaximumBytes() / SceneLoadTaskGraphDetail::DecodedByteReservationWeight)
				    {
					    throw Diagnostics::Error("Scene load exceeded its weighted decode byte budget.");
				    }

				    std::size_t retainedDecodedBytes = rawBytes * SceneLoadTaskGraphDetail::DecodedByteReservationWeight;
				    SceneLoadTaskGraphDetail::ReserveOrThrow(state->Budget, retainedDecodedBytes);
				    work.Payload = SceneAssetPayloadDecoder::Decode(work.Manifest, work.Files);
				    const std::size_t measuredDecodedBytes = SceneLoadPackageBuilder::BuildAssetBlueprints(work);
				    if (measuredDecodedBytes > retainedDecodedBytes)
				    {
					    SceneLoadTaskGraphDetail::ReserveOrThrow(state->Budget, measuredDecodedBytes - retainedDecodedBytes);
				    }
				    else
				    {
					    state->Budget.Release(retainedDecodedBytes - measuredDecodedBytes);
				    }
				    retainedDecodedBytes = measuredDecodedBytes;
				    work.RetainedDecodedBytes = retainedDecodedBytes;
				    state->Budget.Release(work.Files.Reset());
				    work.Manifest = {};
				    state->Budget.Release(work.RetainedManifestBytes);
				    work.RetainedManifestBytes = 0;
				    state->CompletedDecodes.fetch_add(1, std::memory_order_relaxed);
				    return TaskResult::Success();
			    });
			graph.DependsOn(decode, read);
			if (index >= SceneLoadTaskGraphDetail::MaximumSceneAssetsInFlight)
			{
				graph.DependsOn(read, decodedNodes[index - SceneLoadTaskGraphDetail::MaximumSceneAssetsInFlight]);
			}
			decodedNodes.push_back(decode);
		}

		const TaskNodeHandle decoded = graph.WhenAll(TaskDesc{TaskName("Join decoded scene assets"), TaskLane::Background}, decodedNodes);
		graph.ContinueWith(
		    decoded,
		    TaskDesc{TaskName("Finalize scene load package"), TaskLane::Background},
		    [state](TaskExecutionContext& context)
		    {
			    if (context.IsCancellationRequested())
				    return TaskResult::Cancelled("Scene load cancelled before package finalization.");
			    state->Stage.store(LevelLoadOperationStage::Validating, std::memory_order_release);
			    SceneLoadPackageBuilder::Finalize(*state);
			    state->Stage.store(LevelLoadOperationStage::Ready, std::memory_order_release);
			    return TaskResult::Success();
		    });
		return graph.Compile();
	}
}

#include "PCH.h"

#include "World/Systems/GameSystemGraph.h"

#include "Tasks/Public/TaskExecution.h"
#include "Tasks/Public/TaskExecutionContext.h"
#include "Tasks/Public/TaskExecutor.h"
#include "Tasks/Public/TaskGraph.h"

#include <algorithm>
#include <format>
#include <limits>
#include <memory>
#include <queue>
#include <unordered_map>
#include <utility>

namespace
{
	using namespace ECS;

	struct GameSystemExecutionData final
	{
		std::span<const GameSystemExecutionBinding> Bindings;
	};

	struct ResourcePhaseRange final
	{
		GameSystemPhase First;
		GameSystemPhase Last;
	};

	constexpr ResourcePhaseRange GetResourcePhaseRange(GameSystemResourceDomain domain) noexcept
	{
		switch (domain)
		{
			case GameSystemResourceDomain::UpdateInputs:
				return {GameSystemPhase::Simulation, GameSystemPhase::Animation};
			case GameSystemResourceDomain::CameraInputIntent:
				return {GameSystemPhase::Simulation, GameSystemPhase::Simulation};
			case GameSystemResourceDomain::MotionClock:
			case GameSystemResourceDomain::SystemChangeScratch:
				return {GameSystemPhase::Simulation, GameSystemPhase::Deformation};
			case GameSystemResourceDomain::AnimationClips:
			case GameSystemResourceDomain::SkeletonResources:
				return {GameSystemPhase::Animation, GameSystemPhase::Extraction};
			case GameSystemResourceDomain::PoseScratch:
			case GameSystemResourceDomain::MorphScratch:
				return {GameSystemPhase::Animation, GameSystemPhase::Deformation};
			case GameSystemResourceDomain::SkinningOutput:
			case GameSystemResourceDomain::MorphOutput:
				return {GameSystemPhase::Deformation, GameSystemPhase::Extraction};
			case GameSystemResourceDomain::DirtyTransforms:
			case GameSystemResourceDomain::WorldChanges:
				return {GameSystemPhase::Deformation, GameSystemPhase::Extraction};
			case GameSystemResourceDomain::TransformScratch:
			case GameSystemResourceDomain::CameraDerivedScratch:
				return {GameSystemPhase::Transform, GameSystemPhase::Extraction};
			case GameSystemResourceDomain::MeshResources:
			case GameSystemResourceDomain::ExtractionScratch:
			case GameSystemResourceDomain::ExtractionOutput:
			case GameSystemResourceDomain::WorldPublication:
				return {GameSystemPhase::Extraction, GameSystemPhase::Extraction};
		}
		return {GameSystemPhase::Extraction, GameSystemPhase::Simulation};
	}

	bool IsWrite(ComponentAccessMode mode) noexcept { return mode == ComponentAccessMode::Write; }
	bool IsWrite(GameSystemAccessMode mode) noexcept { return mode == GameSystemAccessMode::Write; }

	bool ComponentsConflict(const GameSystemDesc& lhs, const GameSystemDesc& rhs) noexcept
	{
		for (const ComponentAccessDesc& left : lhs.Components)
		{
			for (const ComponentAccessDesc& right : rhs.Components)
			{
				if (left.Type == right.Type && (IsWrite(left.Mode) || IsWrite(right.Mode)))
					return true;
			}
		}
		return false;
	}

	bool ResourcesConflict(const GameSystemDesc& lhs, const GameSystemDesc& rhs) noexcept
	{
		for (const GameSystemResourceAccess& left : lhs.Resources)
		{
			for (const GameSystemResourceAccess& right : rhs.Resources)
			{
				if (left.Domain == right.Domain && (IsWrite(left.Mode) || IsWrite(right.Mode)))
					return true;
			}
		}
		return false;
	}

	bool HasPath(const std::vector<std::vector<std::uint32_t>>& edges, std::uint32_t from, std::uint32_t to)
	{
		std::vector<bool> visited(edges.size());
		std::vector<std::uint32_t> stack{from};
		while (!stack.empty())
		{
			const std::uint32_t current = stack.back();
			stack.pop_back();
			if (current == to)
				return true;
			if (visited[current])
				continue;
			visited[current] = true;
			for (std::uint32_t next : edges[current])
				stack.push_back(next);
		}
		return false;
	}

	bool AddEdge(std::vector<std::vector<std::uint32_t>>& edges, std::uint32_t from, std::uint32_t to)
	{
		std::vector<std::uint32_t>& outgoing = edges[from];
		if (std::find(outgoing.begin(), outgoing.end(), to) != outgoing.end())
			return true;
		outgoing.push_back(to);
		return true;
	}

	std::uint32_t ResolvePartitionCount(std::uint32_t itemCount, const ParallelForPolicy& policy) noexcept
	{
		if (itemCount == 0)
			return 0;
		if (itemCount <= policy.SerialThreshold || itemCount <= policy.GrainSize)
			return 1;
		const std::uint32_t grainPartitions = static_cast<std::uint32_t>(
		    (static_cast<std::uint64_t>(itemCount) + policy.GrainSize - 1u) / policy.GrainSize);
		return (std::min)(policy.MaximumPartitions, grainPartitions);
	}

	TaskResult ExecutePartition(
	    std::uint32_t systemIndex,
	    std::uint32_t partitionIndex,
	    const ParallelForPolicy& policy,
	    TaskExecutionContext& taskContext)
	{
		if (taskContext.IsCancellationRequested())
			return TaskResult::Cancelled("Game-system execution cancelled at the owner boundary.");
		GameSystemExecutionData* execution = taskContext.TryGet<GameSystemExecutionData>();
		if (execution == nullptr || systemIndex >= execution->Bindings.size())
			return TaskResult::Failure("Game-system execution binding is unavailable.");
		const GameSystemExecutionBinding& binding = execution->Bindings[systemIndex];
		if (!binding.GetItemCount || !binding.ExecuteRange)
			return TaskResult::Failure("Game-system execution binding is incomplete.");
		const std::uint32_t itemCount = binding.GetItemCount();
		const std::uint32_t partitionCount = ResolvePartitionCount(itemCount, policy);
		if (partitionIndex >= partitionCount)
			return TaskResult::Success();
		const std::uint32_t partitionSize = static_cast<std::uint32_t>(
		    (static_cast<std::uint64_t>(itemCount) + partitionCount - 1u) / partitionCount);
		const std::uint32_t begin = partitionIndex * partitionSize;
		const std::uint32_t end = (std::min)(itemCount, begin + partitionSize);
		return begin == end || binding.ExecuteRange(begin, end)
		           ? TaskResult::Success()
		           : TaskResult::Failure("Game-system range rejected its declared access or target range.");
	}
}

namespace ECS
{
	struct CompiledGameSystemGraph::Data final
	{
		std::vector<GameSystemDesc> Systems;
		std::vector<std::vector<std::uint32_t>> Edges;
		CompiledTaskGraph Tasks;
		GameSystemGraphError Error;
	};

	CompiledGameSystemGraph::CompiledGameSystemGraph() noexcept = default;
	CompiledGameSystemGraph::~CompiledGameSystemGraph() = default;
	CompiledGameSystemGraph::CompiledGameSystemGraph(CompiledGameSystemGraph&&) noexcept = default;
	CompiledGameSystemGraph& CompiledGameSystemGraph::operator=(CompiledGameSystemGraph&&) noexcept = default;
	CompiledGameSystemGraph::CompiledGameSystemGraph(std::unique_ptr<Data> data) noexcept : m_data(std::move(data)) {}

	bool CompiledGameSystemGraph::IsValid() const noexcept
	{
		return m_data != nullptr && !m_data->Error && m_data->Tasks.IsValid();
	}

	const GameSystemGraphError& CompiledGameSystemGraph::GetError() const noexcept
	{
		static const GameSystemGraphError invalid{GameSystemGraphErrorCode::TaskGraphRejected, "Game-system graph is empty."};
		return m_data != nullptr ? m_data->Error : invalid;
	}

	std::span<const GameSystemDesc> CompiledGameSystemGraph::GetSystems() const noexcept
	{
		return m_data != nullptr ? std::span<const GameSystemDesc>(m_data->Systems) : std::span<const GameSystemDesc>{};
	}

	bool CompiledGameSystemGraph::Execute(
	    TaskExecutor& executor,
	    std::span<const GameSystemExecutionBinding> bindings,
	    GameSystemGraphError& error) const
	{
		if (!IsValid() || bindings.size() != m_data->Systems.size())
		{
			error = {GameSystemGraphErrorCode::BindingMismatch, "Game-system execution bindings do not match the compiled topology."};
			return false;
		}
		for (std::size_t index = 0; index < bindings.size(); ++index)
		{
			if (bindings[index].Id != m_data->Systems[index].Id)
			{
				error = {GameSystemGraphErrorCode::BindingMismatch, "Game-system execution binding identity is stale or reordered."};
				return false;
			}
		}
		GameSystemExecutionData execution{bindings};
		TaskExecutionContext context(execution);
		TaskExecution result = executor.Submit(m_data->Tasks, context);
		if (!result.IsValid() || result.GetStatus() != TaskExecutionStatus::Succeeded)
		{
			error = {GameSystemGraphErrorCode::ExecutionFailed, result.IsValid() ? std::string(result.GetResult().GetMessage())
			                                                                        : "SparkleTasks rejected the game-system execution."};
			return false;
		}
		error = {};
		return true;
	}

	void GameSystemGraph::Add(GameSystemDesc descriptor) { m_systems.push_back(std::move(descriptor)); }

	CompiledGameSystemGraph GameSystemGraph::Compile() const
	{
		auto data = std::make_unique<CompiledGameSystemGraph::Data>();
		data->Systems = m_systems;
		data->Edges.resize(m_systems.size());
		std::unordered_map<std::uint64_t, std::uint32_t> systemById;
		std::unordered_map<std::string, std::uint32_t> systemByName;
		for (std::uint32_t index = 0; index < data->Systems.size(); ++index)
		{
			const GameSystemDesc& system = data->Systems[index];
			if (!system.Id.IsValid())
			{
				data->Error = {GameSystemGraphErrorCode::EmptySystemId, "A game system has an empty stable ID."};
				return CompiledGameSystemGraph(std::move(data));
			}
			if (system.Name.empty())
			{
				data->Error = {GameSystemGraphErrorCode::EmptySystemName, "A game system has an empty name."};
				return CompiledGameSystemGraph(std::move(data));
			}
			if (system.Components.empty() && system.Resources.empty())
			{
				data->Error = {
				    GameSystemGraphErrorCode::UndeclaredAccess,
				    std::format("Game system '{}' declares no component query or resource access.", system.Name)};
				return CompiledGameSystemGraph(std::move(data));
			}
			if (!systemById.emplace(system.Id.Value, index).second || !systemByName.emplace(system.Name, index).second)
			{
				data->Error = {GameSystemGraphErrorCode::DuplicateSystem, std::format("Duplicate game system '{}'.", system.Name)};
				return CompiledGameSystemGraph(std::move(data));
			}
			for (std::size_t left = 0; left < system.Components.size(); ++left)
			{
				for (std::size_t right = left + 1; right < system.Components.size(); ++right)
				{
					if (system.Components[left].Type != system.Components[right].Type)
						continue;
					data->Error = {
					    system.Components[left].Mode == system.Components[right].Mode ? GameSystemGraphErrorCode::DuplicateAccess
					                                                              : GameSystemGraphErrorCode::ConflictingAccessDeclaration,
					    std::format("Game system '{}' declares one component domain more than once.", system.Name)};
					return CompiledGameSystemGraph(std::move(data));
				}
			}
			for (std::size_t left = 0; left < system.Resources.size(); ++left)
			{
				const ResourcePhaseRange range = GetResourcePhaseRange(system.Resources[left].Domain);
				if (system.Phase < range.First || system.Phase > range.Last)
				{
					data->Error = {
					    GameSystemGraphErrorCode::UnavailablePhaseResource,
					    std::format("Game system '{}' declares a resource unavailable in its phase.", system.Name)};
					return CompiledGameSystemGraph(std::move(data));
				}
				for (std::size_t right = left + 1; right < system.Resources.size(); ++right)
				{
					if (system.Resources[left].Domain != system.Resources[right].Domain)
						continue;
					data->Error = {
					    system.Resources[left].Mode == system.Resources[right].Mode ? GameSystemGraphErrorCode::DuplicateAccess
					                                                             : GameSystemGraphErrorCode::ConflictingAccessDeclaration,
					    std::format("Game system '{}' declares one resource domain more than once.", system.Name)};
					return CompiledGameSystemGraph(std::move(data));
				}
			}
			if (system.Execution.Mode == GameSystemExecutionMode::ParallelRanges &&
			    (system.Execution.RangePolicy.GrainSize == 0 || system.Execution.RangePolicy.MaximumPartitions == 0))
			{
				data->Error = {GameSystemGraphErrorCode::TaskGraphRejected, std::format("Game system '{}' has an invalid grain policy.", system.Name)};
				return CompiledGameSystemGraph(std::move(data));
			}
		}

		for (std::uint32_t index = 0; index < data->Systems.size(); ++index)
		{
			const GameSystemDesc& system = data->Systems[index];
			for (GameSystemId prerequisiteId : system.Prerequisites)
			{
				const auto prerequisite = systemById.find(prerequisiteId.Value);
				if (prerequisite == systemById.end())
				{
					data->Error = {GameSystemGraphErrorCode::MissingPrerequisite, std::format("Game system '{}' has a missing prerequisite.", system.Name)};
					return CompiledGameSystemGraph(std::move(data));
				}
				if (data->Systems[prerequisite->second].Phase > system.Phase)
				{
					data->Error = {GameSystemGraphErrorCode::InvalidPhaseDependency, std::format("Game system '{}' depends on a later phase.", system.Name)};
					return CompiledGameSystemGraph(std::move(data));
				}
				AddEdge(data->Edges, prerequisite->second, index);
			}
		}

		for (std::uint32_t left = 0; left < data->Systems.size(); ++left)
		{
			for (std::uint32_t right = left + 1; right < data->Systems.size(); ++right)
			{
				const GameSystemDesc& lhs = data->Systems[left];
				const GameSystemDesc& rhs = data->Systems[right];
				if (lhs.Phase < rhs.Phase)
					AddEdge(data->Edges, left, right);
				else if (rhs.Phase < lhs.Phase)
					AddEdge(data->Edges, right, left);
			}
		}

		for (std::uint32_t left = 0; left < data->Systems.size(); ++left)
		{
			for (std::uint32_t right = left + 1; right < data->Systems.size(); ++right)
			{
				const GameSystemDesc& lhs = data->Systems[left];
				const GameSystemDesc& rhs = data->Systems[right];
				if (lhs.Phase != rhs.Phase || (!ComponentsConflict(lhs, rhs) && !ResourcesConflict(lhs, rhs)))
					continue;
				if (!HasPath(data->Edges, left, right) && !HasPath(data->Edges, right, left))
				{
					data->Error = {
					    GameSystemGraphErrorCode::AmbiguousHazard,
					    std::format("Game systems '{}' and '{}' have an unordered write hazard.", lhs.Name, rhs.Name)};
					return CompiledGameSystemGraph(std::move(data));
				}
			}
		}

		for (std::uint32_t from = 0; from < data->Edges.size(); ++from)
		{
			for (std::uint32_t to : data->Edges[from])
			{
				if (HasPath(data->Edges, to, from))
				{
					data->Error = {GameSystemGraphErrorCode::Cycle, "Game-system prerequisites contain a cycle."};
					return CompiledGameSystemGraph(std::move(data));
				}
			}
		}

		std::uint32_t taskCount = 0;
		for (const GameSystemDesc& system : data->Systems)
			taskCount += system.Execution.Mode == GameSystemExecutionMode::ParallelRanges
			                 ? 1u + system.Execution.RangePolicy.MaximumPartitions
			                 : 1u;
		std::uint32_t edgeCount = 0;
		for (const auto& outgoing : data->Edges)
			edgeCount += static_cast<std::uint32_t>(outgoing.size());
		TaskGraphBuilder tasks(TaskGraphLimits{
		    .MaximumTasks = (std::max)(taskCount, 1u),
		    .MaximumEdges = (std::max)(edgeCount + taskCount, 1u)});
		std::vector<TaskNodeHandle> systemNodes;
		systemNodes.reserve(data->Systems.size());
		for (std::uint32_t systemIndex = 0; systemIndex < data->Systems.size(); ++systemIndex)
		{
			const GameSystemDesc& system = data->Systems[systemIndex];
			const TaskDesc descriptor{TaskName(system.Name), TaskLane::FrameCritical};
			if (system.Execution.Mode == GameSystemExecutionMode::SingleTask)
			{
				systemNodes.push_back(tasks.Add(
				    descriptor,
				    [systemIndex](TaskExecutionContext& context)
				    {
					    return ExecutePartition(systemIndex, 0, ParallelForPolicy{1, (std::numeric_limits<std::uint32_t>::max)(), 1}, context);
				    }));
				continue;
			}
			const TaskNodeHandle group = tasks.Add(descriptor, [](TaskExecutionContext&) { return TaskResult::Success(); });
			systemNodes.push_back(group);
			for (std::uint32_t partition = 0; partition < system.Execution.RangePolicy.MaximumPartitions; ++partition)
			{
				const std::string taskName = std::format("{}.Range{}", system.Name, partition);
				tasks.AddNested(
				    group,
				    TaskDesc{TaskName(taskName), TaskLane::FrameCritical},
				    [systemIndex, partition, policy = system.Execution.RangePolicy](TaskExecutionContext& context)
				    {
					    return ExecutePartition(systemIndex, partition, policy, context);
				    });
			}
		}
		for (std::uint32_t from = 0; from < data->Edges.size(); ++from)
		{
			for (std::uint32_t to : data->Edges[from])
				tasks.DependsOn(systemNodes[to], systemNodes[from]);
		}
		data->Tasks = tasks.Compile();
		if (!data->Tasks)
			data->Error = {GameSystemGraphErrorCode::TaskGraphRejected, data->Tasks.GetError().Message};
		return CompiledGameSystemGraph(std::move(data));
	}
}

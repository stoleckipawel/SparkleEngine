#include "TaskGraph.h"

#include "TaskExecutionContext.h"
#include "TaskGraphStorage.h"

#include <algorithm>
#include <atomic>
#include <functional>
#include <limits>
#include <queue>
#include <utility>
#include <vector>

TaskNodeHandle::TaskNodeHandle() noexcept = default;

TaskNodeHandle::TaskNodeHandle(
    std::uint64_t builderIdentity,
    std::uint32_t builderGeneration,
    std::uint32_t index) noexcept :
	m_builderIdentity(builderIdentity), m_builderGeneration(builderGeneration), m_indexPlusOne(index + 1u)
{
}

bool TaskNodeHandle::operator==(const TaskNodeHandle&) const noexcept = default;

struct TaskGraphBuilder::State final
{
	class Compilation;

	explicit State(TaskGraphLimits requestedLimits) :
		Limits(requestedLimits),
		BuilderIdentity(AcquireBuilderIdentity())
	{
		if (!AreValidLimits(Limits))
		{
			Error = CreateError(
			    TaskGraphErrorCode::InvalidLimits,
			    "Task capacity is zero or a task/edge limit exceeds the hard maximum.");
			return;
		}

		Nodes.reserve(Limits.MaximumTasks);
	}

	static std::uint64_t AcquireBuilderIdentity() noexcept;
	static bool AreValidLimits(TaskGraphLimits limits) noexcept;
	static bool IsValidCompletionPolicy(TaskCompletionPolicy policy) noexcept;
	static bool IsValidTaskLane(TaskLane lane) noexcept;
	static TaskGraphError CreateError(TaskGraphErrorCode code, std::string message);
	static bool HasCycle(const std::vector<std::vector<std::uint32_t>>& adjacency);

	void RecordError(TaskGraphErrorCode code, std::string message)
	{
		if (!Error)
		{
			Error = CreateError(code, std::move(message));
		}
	}

	bool ResolveHandle(TaskNodeHandle handle, std::uint32_t& outIndex)
	{
		if (!handle)
		{
			RecordError(TaskGraphErrorCode::InvalidHandle, "Task graph operation received an invalid task handle.");
			return false;
		}
		if (TaskGraphAccess::GetBuilderIdentity(handle) != BuilderIdentity)
		{
			RecordError(TaskGraphErrorCode::ForeignHandle, "Task graph operation received a handle from another builder.");
			return false;
		}
		if (TaskGraphAccess::GetBuilderGeneration(handle) != BuilderGeneration)
		{
			RecordError(TaskGraphErrorCode::StaleHandle, "Task graph operation received a stale builder-generation handle.");
			return false;
		}

		outIndex = TaskGraphAccess::GetIndex(handle);
		if (outIndex >= Nodes.size())
		{
			RecordError(TaskGraphErrorCode::InvalidHandle, "Task graph handle index is outside the builder task range.");
			return false;
		}
		return true;
	}

	TaskGraphLimits Limits;
	std::uint64_t BuilderIdentity = 0;
	std::uint32_t BuilderGeneration = 1;
	std::uint32_t EdgeCount = 0;
	TaskGraphError Error;
	std::vector<TaskGraphNode> Nodes;
};

class TaskGraphBuilder::State::Compilation final
{
  public:
	explicit Compilation(const State& builder) noexcept;

	std::shared_ptr<TaskGraphStorage> Build() const;

  private:
	void CopyBuilderState(TaskGraphStorage& graph) const;
	bool BuildDependencyRelations(
	    TaskGraphStorage& graph,
	    std::vector<std::vector<std::uint32_t>>& startAdjacency,
	    std::vector<std::vector<std::uint32_t>>& completionAdjacency) const;
	bool ValidatePrerequisiteLane(
	    TaskGraphStorage& graph,
	    std::uint32_t taskIndex,
	    std::uint32_t prerequisiteIndex) const;
	bool ValidateNestedLane(TaskGraphStorage& graph, std::uint32_t taskIndex) const;
	void RejectCycles(
	    TaskGraphStorage& graph,
	    const std::vector<std::vector<std::uint32_t>>& startAdjacency,
	    const std::vector<std::vector<std::uint32_t>>& completionAdjacency) const;

	const State& m_builder;
};

std::uint64_t TaskGraphBuilder::State::AcquireBuilderIdentity() noexcept
{
	static std::atomic_uint64_t nextIdentity{1};
	return nextIdentity.fetch_add(1, std::memory_order_relaxed);
}

bool TaskGraphBuilder::State::AreValidLimits(TaskGraphLimits limits) noexcept
{
	return limits.MaximumTasks > 0 && limits.MaximumTasks <= TaskGraphLimits::HardMaximumTasks &&
	       limits.MaximumEdges <= TaskGraphLimits::HardMaximumEdges;
}

bool TaskGraphBuilder::State::IsValidCompletionPolicy(TaskCompletionPolicy policy) noexcept
{
	return policy == TaskCompletionPolicy::Normal || policy == TaskCompletionPolicy::Cleanup;
}

bool TaskGraphBuilder::State::IsValidTaskLane(TaskLane lane) noexcept
{
	return lane == TaskLane::FrameCritical || lane == TaskLane::Background || lane == TaskLane::BlockingIo;
}

TaskGraphError TaskGraphBuilder::State::CreateError(TaskGraphErrorCode code, std::string message)
{
	return TaskGraphError{.Code = code, .Message = std::move(message)};
}

bool TaskGraphBuilder::State::HasCycle(const std::vector<std::vector<std::uint32_t>>& adjacency)
{
	std::vector<std::uint32_t> incoming(adjacency.size(), 0);
	for (const auto& successors : adjacency)
	{
		for (const std::uint32_t successor : successors)
		{
			++incoming[successor];
		}
	}

	std::priority_queue<std::uint32_t, std::vector<std::uint32_t>, std::greater<>> ready;
	for (std::uint32_t index = 0; index < incoming.size(); ++index)
	{
		if (incoming[index] == 0)
		{
			ready.push(index);
		}
	}

	std::uint32_t visited = 0;
	while (!ready.empty())
	{
		const std::uint32_t index = ready.top();
		ready.pop();
		++visited;

		for (const std::uint32_t successor : adjacency[index])
		{
			if (--incoming[successor] == 0)
			{
				ready.push(successor);
			}
		}
	}

	return visited != adjacency.size();
}

TaskGraphBuilder::State::Compilation::Compilation(const State& builder) noexcept : m_builder(builder) {}

std::shared_ptr<TaskGraphStorage> TaskGraphBuilder::State::Compilation::Build() const
{
	auto graph = std::make_shared<TaskGraphStorage>();
	CopyBuilderState(*graph);
	if (graph->Error)
	{
		return graph;
	}

	const std::size_t taskCount = graph->Nodes.size();
	std::vector<std::vector<std::uint32_t>> startAdjacency(taskCount);
	std::vector<std::vector<std::uint32_t>> completionAdjacency(taskCount);
	if (!BuildDependencyRelations(*graph, startAdjacency, completionAdjacency))
	{
		return graph;
	}

	RejectCycles(*graph, startAdjacency, completionAdjacency);
	return graph;
}

void TaskGraphBuilder::State::Compilation::CopyBuilderState(TaskGraphStorage& graph) const
{
	graph.Error = m_builder.Error;
	graph.Limits = m_builder.Limits;
	graph.BuilderIdentity = m_builder.BuilderIdentity;
	graph.BuilderGeneration = m_builder.BuilderGeneration;
	graph.EdgeCount = m_builder.EdgeCount;
	graph.Nodes = m_builder.Nodes;
}

bool TaskGraphBuilder::State::Compilation::BuildDependencyRelations(
    TaskGraphStorage& graph,
    std::vector<std::vector<std::uint32_t>>& startAdjacency,
    std::vector<std::vector<std::uint32_t>>& completionAdjacency) const
{
	for (std::uint32_t taskIndex = 0; taskIndex < graph.Nodes.size(); ++taskIndex)
	{
		const TaskGraphNode& node = graph.Nodes[taskIndex];
		for (const std::uint32_t prerequisiteIndex : node.Prerequisites)
		{
			if (!ValidatePrerequisiteLane(graph, taskIndex, prerequisiteIndex))
			{
				return false;
			}

			startAdjacency[prerequisiteIndex].push_back(taskIndex);
			completionAdjacency[prerequisiteIndex].push_back(taskIndex);
		}

		if (!ValidateNestedLane(graph, taskIndex))
		{
			return false;
		}
		if (node.Parent.has_value())
		{
			startAdjacency[*node.Parent].push_back(taskIndex);
			completionAdjacency[taskIndex].push_back(*node.Parent);
		}
	}

	return true;
}

bool TaskGraphBuilder::State::Compilation::ValidatePrerequisiteLane(
    TaskGraphStorage& graph,
    std::uint32_t taskIndex,
    std::uint32_t prerequisiteIndex) const
{
	const TaskGraphNode& node = graph.Nodes[taskIndex];
	const TaskGraphNode& prerequisite = graph.Nodes[prerequisiteIndex];
	if (node.Desc.Lane != TaskLane::FrameCritical || prerequisite.Desc.Lane == TaskLane::FrameCritical)
	{
		return true;
	}

	graph.Error = CreateError(
	    TaskGraphErrorCode::InvalidLaneDependency,
	    "A FrameCritical task cannot depend on Background or BlockingIo work.");
	return false;
}

bool TaskGraphBuilder::State::Compilation::ValidateNestedLane(
    TaskGraphStorage& graph,
    std::uint32_t taskIndex) const
{
	const TaskGraphNode& node = graph.Nodes[taskIndex];
	if (!node.Parent.has_value())
	{
		return true;
	}

	const TaskGraphNode& parent = graph.Nodes[*node.Parent];
	if (node.Desc.Lane == parent.Desc.Lane ||
	    (node.Desc.Lane != TaskLane::FrameCritical && parent.Desc.Lane != TaskLane::FrameCritical))
	{
		return true;
	}

	graph.Error = CreateError(
	    TaskGraphErrorCode::InvalidLaneDependency,
	    "FrameCritical parent and nested task completion must remain in the FrameCritical lane.");
	return false;
}

void TaskGraphBuilder::State::Compilation::RejectCycles(
    TaskGraphStorage& graph,
    const std::vector<std::vector<std::uint32_t>>& startAdjacency,
    const std::vector<std::vector<std::uint32_t>>& completionAdjacency) const
{
	if (!HasCycle(startAdjacency) && !HasCycle(completionAdjacency))
	{
		return;
	}

	graph.Error = CreateError(
	    TaskGraphErrorCode::Cycle,
	    "Task graph contains a dependency or nested-completion cycle.");
}

CompiledTaskGraph::CompiledTaskGraph() noexcept = default;

std::uint64_t TaskGraphAccess::GetBuilderIdentity(TaskNodeHandle handle) noexcept
{
	return handle.m_builderIdentity;
}

std::uint32_t TaskGraphAccess::GetBuilderGeneration(TaskNodeHandle handle) noexcept
{
	return handle.m_builderGeneration;
}

std::uint32_t TaskGraphAccess::GetIndex(TaskNodeHandle handle) noexcept
{
	return handle.m_indexPlusOne - 1u;
}

bool TaskGraphAccess::Decode(
    TaskNodeHandle handle,
    std::uint64_t builderIdentity,
    std::uint32_t builderGeneration,
    std::uint32_t taskCount,
    std::uint32_t& outIndex) noexcept
{
	if (!handle || handle.m_builderIdentity != builderIdentity || handle.m_builderGeneration != builderGeneration)
	{
		return false;
	}
	outIndex = handle.m_indexPlusOne - 1u;
	return outIndex < taskCount;
}

void TaskGraphAccess::RecordError(TaskGraphBuilder& builder, TaskGraphErrorCode code, std::string message)
{
	builder.m_state->RecordError(code, std::move(message));
}

CompiledTaskGraph::CompiledTaskGraph(std::shared_ptr<const TaskGraphStorage> data) noexcept : m_data(std::move(data)) {}

bool CompiledTaskGraph::IsValid() const noexcept
{
	return m_data != nullptr && !m_data->Error;
}

const TaskGraphError& CompiledTaskGraph::GetError() const noexcept
{
	static const TaskGraphError invalidGraphError{
	    .Code = TaskGraphErrorCode::InvalidHandle,
	    .Message = "No compiled task graph is present."};
	return m_data != nullptr ? m_data->Error : invalidGraphError;
}

std::uint32_t CompiledTaskGraph::GetTaskCount() const noexcept
{
	return m_data != nullptr ? static_cast<std::uint32_t>(m_data->Nodes.size()) : 0;
}

std::uint32_t CompiledTaskGraph::GetEdgeCount() const noexcept
{
	return m_data != nullptr ? m_data->EdgeCount : 0;
}

TaskGraphBuilder::TaskGraphBuilder(TaskGraphLimits limits) : m_state(std::make_unique<State>(limits)) {}

TaskGraphBuilder::~TaskGraphBuilder() = default;

TaskNodeHandle TaskGraphBuilder::Add(TaskDesc desc, TaskFunction function)
{
	if (m_state->Error)
	{
		return {};
	}
	if (!desc.Name.IsValid())
	{
		m_state->RecordError(TaskGraphErrorCode::InvalidTaskName, "Task names must be non-empty and at most 96 bytes.");
		return {};
	}
	if (!State::IsValidTaskLane(desc.Lane))
	{
		m_state->RecordError(TaskGraphErrorCode::InvalidTaskLane, "Task lane is not recognized.");
		return {};
	}
	if (!State::IsValidCompletionPolicy(desc.CompletionPolicy))
	{
		m_state->RecordError(TaskGraphErrorCode::InvalidCompletionPolicy, "Task completion policy is not recognized.");
		return {};
	}
	if (m_state->Nodes.size() >= m_state->Limits.MaximumTasks)
	{
		m_state->RecordError(TaskGraphErrorCode::TaskCapacityExceeded, "Task graph exceeded its configured task capacity.");
		return {};
	}

	const std::uint32_t index = static_cast<std::uint32_t>(m_state->Nodes.size());
	m_state->Nodes.push_back(TaskGraphNode{.Desc = std::move(desc), .Function = std::move(function)});
	return TaskNodeHandle(m_state->BuilderIdentity, m_state->BuilderGeneration, index);
}

TaskNodeHandle TaskGraphBuilder::AddNested(TaskNodeHandle parent, TaskDesc desc, TaskFunction function)
{
	if (m_state->Error)
	{
		return {};
	}
	std::uint32_t parentIndex = 0;
	if (!m_state->ResolveHandle(parent, parentIndex))
	{
		return {};
	}
	if (m_state->EdgeCount >= m_state->Limits.MaximumEdges)
	{
		m_state->RecordError(TaskGraphErrorCode::EdgeCapacityExceeded, "Task graph exceeded its configured edge capacity.");
		return {};
	}

	TaskNodeHandle child = Add(std::move(desc), std::move(function));
	if (!child)
	{
		return {};
	}
	const std::uint32_t childIndex = child.m_indexPlusOne - 1u;
	m_state->Nodes[childIndex].Parent = parentIndex;
	m_state->Nodes[parentIndex].NestedChildren.push_back(childIndex);
	++m_state->EdgeCount;
	return child;
}

bool TaskGraphBuilder::DependsOn(TaskNodeHandle task, TaskNodeHandle prerequisite)
{
	if (m_state->Error)
	{
		return false;
	}
	std::uint32_t taskIndex = 0;
	std::uint32_t prerequisiteIndex = 0;
	if (!m_state->ResolveHandle(task, taskIndex) || !m_state->ResolveHandle(prerequisite, prerequisiteIndex))
	{
		return false;
	}
	if (taskIndex == prerequisiteIndex)
	{
		m_state->RecordError(TaskGraphErrorCode::SelfDependency, "A task cannot depend on itself.");
		return false;
	}
	if (m_state->EdgeCount >= m_state->Limits.MaximumEdges)
	{
		m_state->RecordError(TaskGraphErrorCode::EdgeCapacityExceeded, "Task graph exceeded its configured edge capacity.");
		return false;
	}

	auto& prerequisites = m_state->Nodes[taskIndex].Prerequisites;
	if (std::find(prerequisites.begin(), prerequisites.end(), prerequisiteIndex) != prerequisites.end())
	{
		m_state->RecordError(TaskGraphErrorCode::DuplicateDependency, "A task dependency edge was added more than once.");
		return false;
	}

	prerequisites.push_back(prerequisiteIndex);
	m_state->Nodes[prerequisiteIndex].Dependents.push_back(taskIndex);
	++m_state->EdgeCount;
	return true;
}

TaskNodeHandle TaskGraphBuilder::WhenAll(TaskDesc desc, std::span<const TaskNodeHandle> prerequisites)
{
	TaskNodeHandle barrier = Add(std::move(desc), [](TaskExecutionContext&) { return TaskResult::Success(); });
	for (const TaskNodeHandle prerequisite : prerequisites)
	{
		DependsOn(barrier, prerequisite);
	}
	return barrier;
}

TaskNodeHandle TaskGraphBuilder::WhenAll(TaskName name, std::span<const TaskNodeHandle> prerequisites)
{
	return WhenAll(TaskDesc{.Name = std::move(name)}, prerequisites);
}

TaskNodeHandle TaskGraphBuilder::ContinueWith(TaskNodeHandle prerequisite, TaskDesc desc, TaskFunction function)
{
	TaskNodeHandle continuation = Add(std::move(desc), std::move(function));
	DependsOn(continuation, prerequisite);
	return continuation;
}

CompiledTaskGraph TaskGraphBuilder::Compile() const
{
	return CompiledTaskGraph(State::Compilation(*m_state).Build());
}

void TaskGraphBuilder::Reset() noexcept
{
	m_state->Nodes.clear();
	m_state->EdgeCount = 0;
	m_state->Error = {};
	if (!State::AreValidLimits(m_state->Limits))
	{
		m_state->RecordError(TaskGraphErrorCode::InvalidLimits, "Task capacity is zero or a task/edge limit exceeds the hard maximum.");
		return;
	}
	if (m_state->BuilderGeneration == std::numeric_limits<std::uint32_t>::max())
	{
		m_state->RecordError(TaskGraphErrorCode::GenerationExhausted, "Task graph builder generation is exhausted and cannot be reused.");
		return;
	}
	++m_state->BuilderGeneration;
}

const TaskGraphError& TaskGraphBuilder::GetError() const noexcept
{
	return m_state->Error;
}

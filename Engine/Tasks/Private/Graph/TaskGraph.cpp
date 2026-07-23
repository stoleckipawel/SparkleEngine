#include "TaskGraph.h"

#include "TaskExecutionContext.h"
#include "TaskGraphInternal.h"

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

class TaskGraphOperations final
{
  public:
	static std::uint64_t AcquireBuilderIdentity() noexcept
	{
		static std::atomic_uint64_t nextIdentity{1};
		return nextIdentity.fetch_add(1, std::memory_order_relaxed);
	}

	static bool IsValidCompletionPolicy(TaskCompletionPolicy policy) noexcept
	{
		return policy == TaskCompletionPolicy::Normal || policy == TaskCompletionPolicy::Cleanup;
	}

	static bool IsValidTaskLane(TaskLane lane) noexcept
	{
		return lane == TaskLane::FrameCritical || lane == TaskLane::Background || lane == TaskLane::BlockingIo;
	}

	static TaskGraphError MakeError(TaskGraphErrorCode code, std::string message)
	{
		return TaskGraphError{.Code = code, .Message = std::move(message)};
	}

	static bool AreValidLimits(TaskGraphLimits limits) noexcept
	{
		return limits.MaximumTasks > 0 && limits.MaximumTasks <= TaskGraphLimits::HardMaximumTasks &&
		       limits.MaximumEdges <= TaskGraphLimits::HardMaximumEdges;
	}

	static bool HasCycle(const std::vector<std::vector<std::uint32_t>>& adjacency)
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
};

struct TaskGraphBuilder::State final
{
	explicit State(TaskGraphLimits requestedLimits) : Limits(requestedLimits), BuilderIdentity(TaskGraphOperations::AcquireBuilderIdentity())
	{
		if (!TaskGraphOperations::AreValidLimits(Limits))
		{
			Error = TaskGraphOperations::MakeError(TaskGraphErrorCode::InvalidLimits, "Task capacity is zero or a task/edge limit exceeds the hard maximum.");
			return;
		}

		Nodes.reserve(Limits.MaximumTasks);
	}

	void RecordError(TaskGraphErrorCode code, std::string message)
	{
		if (!Error)
		{
			Error = TaskGraphOperations::MakeError(code, std::move(message));
		}
	}

	bool ResolveHandle(TaskNodeHandle handle, std::uint32_t& outIndex)
	{
		if (!handle)
		{
			RecordError(TaskGraphErrorCode::InvalidHandle, "Task graph operation received an invalid task handle.");
			return false;
		}
		if (TaskDetail::TaskGraphAccess::GetBuilderIdentity(handle) != BuilderIdentity)
		{
			RecordError(TaskGraphErrorCode::ForeignHandle, "Task graph operation received a handle from another builder.");
			return false;
		}
		if (TaskDetail::TaskGraphAccess::GetBuilderGeneration(handle) != BuilderGeneration)
		{
			RecordError(TaskGraphErrorCode::StaleHandle, "Task graph operation received a stale builder-generation handle.");
			return false;
		}

		outIndex = TaskDetail::TaskGraphAccess::GetIndex(handle);
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
	std::vector<TaskDetail::CompiledTaskNode> Nodes;
};

CompiledTaskGraph::CompiledTaskGraph() noexcept = default;

std::uint64_t TaskDetail::TaskGraphAccess::GetBuilderIdentity(TaskNodeHandle handle) noexcept
{
	return handle.m_builderIdentity;
}

std::uint32_t TaskDetail::TaskGraphAccess::GetBuilderGeneration(TaskNodeHandle handle) noexcept
{
	return handle.m_builderGeneration;
}

std::uint32_t TaskDetail::TaskGraphAccess::GetIndex(TaskNodeHandle handle) noexcept
{
	return handle.m_indexPlusOne - 1u;
}

bool TaskDetail::TaskGraphAccess::Decode(
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

void TaskDetail::TaskGraphAccess::RecordError(TaskGraphBuilder& builder, TaskGraphErrorCode code, std::string message)
{
	builder.m_state->RecordError(code, std::move(message));
}

CompiledTaskGraph::CompiledTaskGraph(std::shared_ptr<const TaskDetail::CompiledTaskGraphData> data) noexcept : m_data(std::move(data)) {}

bool CompiledTaskGraph::IsValid() const noexcept
{
	return m_data != nullptr && !m_data->Error;
}

const TaskGraphError& CompiledTaskGraph::GetError() const noexcept
{
	static const TaskGraphError invalidGraphError = TaskGraphOperations::MakeError(TaskGraphErrorCode::InvalidHandle, "No compiled task graph is present.");
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
	if (!TaskGraphOperations::IsValidTaskLane(desc.Lane))
	{
		m_state->RecordError(TaskGraphErrorCode::InvalidTaskLane, "Task lane is not recognized.");
		return {};
	}
	if (!TaskGraphOperations::IsValidCompletionPolicy(desc.CompletionPolicy))
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
	m_state->Nodes.push_back(TaskDetail::CompiledTaskNode{.Desc = std::move(desc), .Function = std::move(function)});
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
	auto data = std::make_shared<TaskDetail::CompiledTaskGraphData>();
	data->Limits = m_state->Limits;
	data->BuilderIdentity = m_state->BuilderIdentity;
	data->BuilderGeneration = m_state->BuilderGeneration;
	data->EdgeCount = m_state->EdgeCount;

	if (m_state->Error)
	{
		data->Error = m_state->Error;
		return CompiledTaskGraph(std::move(data));
	}

	data->Nodes = m_state->Nodes;
	const std::size_t taskCount = data->Nodes.size();
	std::vector<std::vector<std::uint32_t>> startAdjacency(taskCount);
	std::vector<std::vector<std::uint32_t>> completionAdjacency(taskCount);

	// Nested work starts after its parent body but completes before its parent group.
	// Checking both directions rejects graphs that can build but can never settle.
	for (std::uint32_t taskIndex = 0; taskIndex < taskCount; ++taskIndex)
	{
		const auto& node = data->Nodes[taskIndex];
		for (const std::uint32_t prerequisite : node.Prerequisites)
		{
			if (node.Desc.Lane == TaskLane::FrameCritical && data->Nodes[prerequisite].Desc.Lane != TaskLane::FrameCritical)
			{
				data->Error = TaskGraphOperations::MakeError(
				    TaskGraphErrorCode::InvalidLaneDependency,
				    "A FrameCritical task cannot depend on Background or BlockingIo work.");
				return CompiledTaskGraph(std::move(data));
			}
			startAdjacency[prerequisite].push_back(taskIndex);
			completionAdjacency[prerequisite].push_back(taskIndex);
		}
		if (node.Parent.has_value())
		{
			const auto& parent = data->Nodes[*node.Parent];
			if (node.Desc.Lane != parent.Desc.Lane &&
			    (node.Desc.Lane == TaskLane::FrameCritical || parent.Desc.Lane == TaskLane::FrameCritical))
			{
				data->Error = TaskGraphOperations::MakeError(
				    TaskGraphErrorCode::InvalidLaneDependency,
				    "FrameCritical parent and nested task completion must remain in the FrameCritical lane.");
				return CompiledTaskGraph(std::move(data));
			}
			startAdjacency[*node.Parent].push_back(taskIndex);
			completionAdjacency[taskIndex].push_back(*node.Parent);
		}
	}

	if (TaskGraphOperations::HasCycle(startAdjacency) || TaskGraphOperations::HasCycle(completionAdjacency))
	{
		data->Error = TaskGraphOperations::MakeError(
		    TaskGraphErrorCode::Cycle,
		    "Task graph contains a dependency or nested-completion cycle.");
	}

	return CompiledTaskGraph(std::move(data));
}

void TaskGraphBuilder::Reset() noexcept
{
	m_state->Nodes.clear();
	m_state->EdgeCount = 0;
	m_state->Error = {};
	if (!TaskGraphOperations::AreValidLimits(m_state->Limits))
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

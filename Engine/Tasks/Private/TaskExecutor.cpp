#include "TaskExecutor.h"

#include "TaskGraphInternal.h"

#include <exception>
#include <format>
#include <queue>
#include <utility>
#include <vector>

struct TaskExecution::State final
{
	std::uint64_t Generation = 0;
	std::uint64_t BuilderIdentity = 0;
	std::uint32_t BuilderGeneration = 0;
	TaskExecutionStatus Status = TaskExecutionStatus::Invalid;
	TaskResult Result = TaskResult::Cancelled("Invalid task execution.");
	std::string FirstFailureTaskName;
	std::vector<TaskResult> TaskResults;
	std::vector<bool> Settled;
	std::uint32_t SettledTaskCount = 0;
};

namespace
{
	struct RuntimeTaskState final
	{
		std::uint32_t RemainingPrerequisites = 0;
		std::uint32_t UnfinishedCount = 1;
		bool ParentBodyComplete = true;
		bool BlockedByPrerequisite = false;
		bool BlockedByParent = false;
		bool Scheduled = false;
		bool BodyComplete = false;
		bool Terminal = false;
		TaskResult AggregateResult = TaskResult::Success();
	};

	TaskResult InvokeTask(const TaskDetail::CompiledTaskNode& node, TaskExecutionContext& context)
	{
		if (!node.Function)
		{
			return TaskResult::Success();
		}

		try
		{
			return node.Function(context);
		}
		catch (const std::exception& exception)
		{
			return TaskResult::Failure(std::format("Unhandled task exception: {}", exception.what()));
		}
		catch (...)
		{
			return TaskResult::Failure("Unhandled non-standard task exception.");
		}
	}
}

TaskExecution::TaskExecution(std::unique_ptr<State> state) noexcept : m_state(std::move(state)) {}

TaskExecution::TaskExecution() noexcept = default;

TaskExecution::~TaskExecution() = default;

TaskExecution::TaskExecution(TaskExecution&&) noexcept = default;

TaskExecution& TaskExecution::operator=(TaskExecution&&) noexcept = default;

bool TaskExecution::IsValid() const noexcept
{
	return m_state != nullptr;
}

std::uint64_t TaskExecution::GetGeneration() const noexcept
{
	return m_state != nullptr ? m_state->Generation : 0;
}

TaskExecutionStatus TaskExecution::GetStatus() const noexcept
{
	return m_state != nullptr ? m_state->Status : TaskExecutionStatus::Invalid;
}

const TaskResult& TaskExecution::GetResult() const noexcept
{
	static const TaskResult invalidResult = TaskResult::Cancelled("Invalid task execution.");
	return m_state != nullptr ? m_state->Result : invalidResult;
}

std::string_view TaskExecution::GetFirstFailureTaskName() const noexcept
{
	return m_state != nullptr ? std::string_view(m_state->FirstFailureTaskName) : std::string_view{};
}

std::optional<TaskResult> TaskExecution::GetTaskResult(TaskNodeHandle handle) const
{
	if (m_state == nullptr)
	{
		return std::nullopt;
	}

	std::uint32_t index = 0;
	if (!TaskDetail::TaskGraphAccess::Decode(
	        handle,
	        m_state->BuilderIdentity,
	        m_state->BuilderGeneration,
	        static_cast<std::uint32_t>(m_state->TaskResults.size()),
	        index) ||
	    !m_state->Settled[index])
	{
		return std::nullopt;
	}
	return m_state->TaskResults[index];
}

std::uint32_t TaskExecution::GetSettledTaskCount() const noexcept
{
	return m_state != nullptr ? m_state->SettledTaskCount : 0;
}

TaskExecutor::TaskExecutor(TaskExecutorConfig config) noexcept : m_config(config) {}

TaskExecution TaskExecutor::Submit(const CompiledTaskGraph& graph, TaskExecutionContext& context)
{
	const std::uint64_t generation = m_nextExecutionGeneration++;
	context.SetExecutionGeneration(generation);
	const auto reject = [generation](std::string_view reason)
	{
		auto state = std::make_unique<TaskExecution::State>();
		state->Generation = generation;
		state->Status = TaskExecutionStatus::Rejected;
		state->Result = TaskResult::Failure(reason);
		return TaskExecution(std::move(state));
	};

	if (!graph.IsValid())
	{
		return reject(graph.GetError().Message);
	}

	const auto& compiled = *graph.m_data;
	if (compiled.Nodes.size() > m_config.MaximumTasksPerExecution || compiled.EdgeCount > m_config.MaximumEdgesPerExecution)
	{
		return reject("Compiled task graph exceeds this executor's bounded execution capacity.");
	}

	auto execution = std::make_unique<TaskExecution::State>();
	execution->Generation = generation;
	execution->BuilderIdentity = compiled.BuilderIdentity;
	execution->BuilderGeneration = compiled.BuilderGeneration;
	execution->TaskResults.resize(compiled.Nodes.size());
	execution->Settled.resize(compiled.Nodes.size(), false);

	std::vector<RuntimeTaskState> runtime(compiled.Nodes.size());
	for (std::uint32_t index = 0; index < compiled.Nodes.size(); ++index)
	{
		runtime[index].RemainingPrerequisites = static_cast<std::uint32_t>(compiled.Nodes[index].Prerequisites.size());
		runtime[index].UnfinishedCount += static_cast<std::uint32_t>(compiled.Nodes[index].NestedChildren.size());
		runtime[index].ParentBodyComplete = !compiled.Nodes[index].Parent.has_value();
	}

	std::priority_queue<std::uint32_t, std::vector<std::uint32_t>, std::greater<>> ready;
	auto trySchedule = [&](std::uint32_t index)
	{
		RuntimeTaskState& task = runtime[index];
		if (!task.Scheduled && !task.BodyComplete && task.RemainingPrerequisites == 0 && task.ParentBodyComplete)
		{
			task.Scheduled = true;
			ready.push(index);
		}
	};

	for (std::uint32_t index = 0; index < compiled.Nodes.size(); ++index)
	{
		trySchedule(index);
	}

	bool observedCancellation = false;
	std::function<void(std::uint32_t)> completeTask;
	completeTask = [&](std::uint32_t index)
	{
		RuntimeTaskState& completed = runtime[index];
		if (completed.Terminal)
		{
			return;
		}
		completed.Terminal = true;
		execution->TaskResults[index] = completed.AggregateResult;
		execution->Settled[index] = true;
		++execution->SettledTaskCount;

		for (const std::uint32_t dependentIndex : compiled.Nodes[index].Dependents)
		{
			RuntimeTaskState& dependent = runtime[dependentIndex];
			dependent.BlockedByPrerequisite |= !completed.AggregateResult.Succeeded();
			--dependent.RemainingPrerequisites;
			trySchedule(dependentIndex);
		}

		if (compiled.Nodes[index].Parent.has_value())
		{
			const std::uint32_t parentIndex = *compiled.Nodes[index].Parent;
			RuntimeTaskState& parent = runtime[parentIndex];
			if (!completed.AggregateResult.Succeeded() && parent.AggregateResult.Succeeded())
			{
				parent.AggregateResult = completed.AggregateResult;
			}
			if (--parent.UnfinishedCount == 0)
			{
				completeTask(parentIndex);
			}
		}
	};

	while (!ready.empty())
	{
		const std::uint32_t index = ready.top();
		ready.pop();
		RuntimeTaskState& task = runtime[index];
		const TaskDetail::CompiledTaskNode& node = compiled.Nodes[index];

		const bool blocked = task.BlockedByPrerequisite || task.BlockedByParent;
		TaskResult bodyResult = blocked && node.Desc.CompletionPolicy == TaskCompletionPolicy::Normal
		                            ? TaskResult::Cancelled("A prerequisite or nested parent did not succeed.")
		                            : InvokeTask(node, context);

		task.BodyComplete = true;
		task.AggregateResult = bodyResult;
		if (bodyResult.Failed() && execution->FirstFailureTaskName.empty())
		{
			execution->FirstFailureTaskName = std::string(node.Desc.Name.Get());
			execution->Result = bodyResult;
		}
		observedCancellation |= bodyResult.WasCancelled();

		for (const std::uint32_t childIndex : node.NestedChildren)
		{
			RuntimeTaskState& child = runtime[childIndex];
			child.ParentBodyComplete = true;
			child.BlockedByParent = !bodyResult.Succeeded();
			trySchedule(childIndex);
		}

		if (--task.UnfinishedCount == 0)
		{
			completeTask(index);
		}
	}

	if (execution->SettledTaskCount != compiled.Nodes.size())
	{
		return reject("Compiled task graph could not settle all nodes in serial execution.");
	}

	if (!execution->FirstFailureTaskName.empty())
	{
		execution->Status = TaskExecutionStatus::Failed;
	}
	else if (observedCancellation)
	{
		execution->Status = TaskExecutionStatus::Cancelled;
		execution->Result = TaskResult::Cancelled("Task execution contained cancellation.");
	}
	else
	{
		execution->Status = TaskExecutionStatus::Succeeded;
		execution->Result = TaskResult::Success();
	}

	return TaskExecution(std::move(execution));
}

TaskExecution TaskExecutor::Submit(TaskDesc desc, TaskFunction function, TaskExecutionContext& context)
{
	TaskGraphBuilder builder(TaskGraphLimits{
	    .MaximumTasks = m_config.MaximumTasksPerExecution,
	    .MaximumEdges = m_config.MaximumEdgesPerExecution});
	builder.Add(std::move(desc), std::move(function));
	return Submit(builder.Compile(), context);
}

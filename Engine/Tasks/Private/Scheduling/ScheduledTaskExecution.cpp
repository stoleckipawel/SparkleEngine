#include "ScheduledTaskExecution.h"

#include "Execution/TaskFunctionInvoker.h"
#include "Profiling/TaskProfiler.h"

#include <utility>

TaskExecutor::Implementation::Runtime::ScheduledTaskExecution::ScheduledTaskExecution(
    Runtime& owner,
    std::shared_ptr<const TaskGraphStorage> graph,
    TaskExecutionContext context,
    std::shared_ptr<TaskExecution::State> execution) :
	m_owner(owner),
	m_graph(std::move(graph)),
	m_context(std::move(context)),
	m_execution(std::move(execution)),
	m_generation(m_execution->Data.Generation),
	m_tasks(std::make_unique<ScheduledTaskState[]>(m_graph->Nodes.size())),
	m_taskResults(m_graph->Nodes.size()),
	m_settled(m_graph->Nodes.size(), false)
{
	InitializeTaskStates();
}

void TaskExecutor::Implementation::Runtime::ScheduledTaskExecution::InitializeTaskStates()
{
	for (std::uint32_t index = 0; index < m_graph->Nodes.size(); ++index)
	{
		const TaskGraphNode& node = m_graph->Nodes[index];
		ScheduledTaskState& task = m_tasks[index];

		task.RemainingPrerequisites.store(
		    static_cast<std::uint32_t>(node.Prerequisites.size()),
		    std::memory_order_relaxed);
		task.UnfinishedCount.store(
		    1u + static_cast<std::uint32_t>(node.NestedChildren.size()),
		    std::memory_order_relaxed);
		task.ParentBodyComplete.store(!node.Parent.has_value(), std::memory_order_relaxed);
	}
}

void TaskExecutor::Implementation::Runtime::ScheduledTaskExecution::Start()
{
	RecordDependencies();
	if (m_graph->Nodes.empty())
	{
		Finish();
		return;
	}

	ScheduleInitialTasks();
}

void TaskExecutor::Implementation::Runtime::ScheduledTaskExecution::RecordDependencies() const
{
	for (std::uint32_t dependent = 0; dependent < m_graph->Nodes.size(); ++dependent)
	{
		for (const std::uint32_t prerequisite : m_graph->Nodes[dependent].Prerequisites)
		{
			TaskProfiler::RecordDependency(m_generation, prerequisite, dependent);
		}
	}
}

void TaskExecutor::Implementation::Runtime::ScheduledTaskExecution::ScheduleInitialTasks()
{
	for (std::uint32_t index = 0; index < m_graph->Nodes.size(); ++index)
	{
		TrySchedule(index, nullptr);
	}
}

void TaskExecutor::Implementation::Runtime::ScheduledTaskExecution::Execute(
    std::uint32_t index,
    TaskWorker& worker)
{
	ScheduledTaskState& task = m_tasks[index];
	const TaskGraphNode& node = m_graph->Nodes[index];
	const std::stop_token cancellation = m_execution->Cancellation.get_token();
	const TaskProfiler::TimePoint taskStart =
	    TaskProfiler::Begin(node.Desc, m_generation, index, worker.LaneWorkerIndex);

	const bool blocked = task.BlockedByPrerequisite.load(std::memory_order_acquire) ||
	                     task.BlockedByParent.load(std::memory_order_acquire) ||
	                     cancellation.stop_requested();
	TaskExecutionContext taskContext = m_context;
	TaskExecutionContextBinding::Bind(taskContext, m_generation, node.Desc.Lane, cancellation);

	const TaskResult result =
	    blocked && node.Desc.CompletionPolicy == TaskCompletionPolicy::Normal
	        ? TaskResult::Cancelled("Task execution was cancelled or a prerequisite did not succeed.")
	        : TaskFunctionInvoker::Invoke(node, taskContext);

	RecordTaskResult(index, node, result);
	TaskProfiler::End(node.Desc, m_generation, index, worker.LaneWorkerIndex, result, taskStart);

	ReleaseNestedTasks(node, result, worker);
	ReleaseUnfinished(index, &worker);
}

void TaskExecutor::Implementation::Runtime::ScheduledTaskExecution::RecordTaskResult(
    std::uint32_t index,
    const TaskGraphNode& node,
    const TaskResult& result)
{
	std::lock_guard lock(m_resultMutex);
	m_taskResults[index] = result;
	if (result.Failed() && m_firstFailureTaskName.empty())
	{
		m_firstFailureTaskName = std::string(node.Desc.Name.Get());
		m_firstFailure = result;
	}
	if (result.WasCancelled())
	{
		m_observedCancellation.store(true, std::memory_order_release);
	}
}

void TaskExecutor::Implementation::Runtime::ScheduledTaskExecution::ReleaseNestedTasks(
    const TaskGraphNode& node,
    const TaskResult& result,
    TaskWorker& worker)
{
	for (const std::uint32_t childIndex : node.NestedChildren)
	{
		ScheduledTaskState& child = m_tasks[childIndex];
		if (!result.Succeeded())
		{
			child.BlockedByParent.store(true, std::memory_order_release);
		}

		child.ParentBodyComplete.store(true, std::memory_order_release);
		TrySchedule(childIndex, &worker);
	}
}

void TaskExecutor::Implementation::Runtime::ScheduledTaskExecution::TrySchedule(
    std::uint32_t index,
    TaskWorker* preferredWorker)
{
	ScheduledTaskState& task = m_tasks[index];
	if (task.RemainingPrerequisites.load(std::memory_order_acquire) != 0 ||
	    !task.ParentBodyComplete.load(std::memory_order_acquire))
	{
		return;
	}

	bool expected = false;
	if (task.Scheduled.compare_exchange_strong(expected, true, std::memory_order_acq_rel))
	{
		const TaskLane lane = m_graph->Nodes[index].Desc.Lane;
		m_owner.Enqueue(
		    ReadyTask{.Execution = shared_from_this(), .TaskIndex = index},
		    preferredWorker != nullptr && preferredWorker->Lane == lane ? preferredWorker : nullptr,
		    lane);
	}
}

void TaskExecutor::Implementation::Runtime::ScheduledTaskExecution::ReleaseUnfinished(
    std::uint32_t index,
    TaskWorker* worker)
{
	if (m_tasks[index].UnfinishedCount.fetch_sub(1, std::memory_order_acq_rel) == 1)
	{
		CompleteLogical(index, worker);
	}
}

void TaskExecutor::Implementation::Runtime::ScheduledTaskExecution::CompleteLogical(
    std::uint32_t index,
    TaskWorker* worker)
{
	bool expected = false;
	if (!m_tasks[index].Terminal.compare_exchange_strong(expected, true, std::memory_order_acq_rel))
	{
		return;
	}

	TaskResult completedResult;
	{
		std::lock_guard lock(m_resultMutex);
		completedResult = m_taskResults[index];
		m_settled[index] = true;
	}

	for (const std::uint32_t dependentIndex : m_graph->Nodes[index].Dependents)
	{
		ScheduledTaskState& dependent = m_tasks[dependentIndex];
		if (!completedResult.Succeeded())
		{
			dependent.BlockedByPrerequisite.store(true, std::memory_order_release);
		}
		if (dependent.RemainingPrerequisites.fetch_sub(1, std::memory_order_acq_rel) == 1)
		{
			TrySchedule(dependentIndex, worker);
		}
	}

	if (m_graph->Nodes[index].Parent.has_value())
	{
		const std::uint32_t parentIndex = *m_graph->Nodes[index].Parent;
		if (!completedResult.Succeeded())
		{
			std::lock_guard lock(m_resultMutex);
			if (m_taskResults[parentIndex].Succeeded())
			{
				m_taskResults[parentIndex] = completedResult;
			}
		}

		ReleaseUnfinished(parentIndex, worker);
	}

	if (m_settledTaskCount.fetch_add(1, std::memory_order_acq_rel) + 1u == m_graph->Nodes.size())
	{
		Finish();
	}
}

TaskExecutionCompletion TaskExecutor::Implementation::Runtime::ScheduledTaskExecution::BuildCompletion()
{
	TaskExecutionCompletion completion;
	completion.Generation = m_generation;
	completion.BuilderIdentity = m_graph->BuilderIdentity;
	completion.BuilderGeneration = m_graph->BuilderGeneration;
	completion.SettledTaskCount = m_settledTaskCount.load(std::memory_order_acquire);

	{
		std::lock_guard lock(m_resultMutex);
		completion.TaskResults = m_taskResults;
		completion.Settled = m_settled;
		completion.FirstFailureTaskName = m_firstFailureTaskName;
		if (!m_firstFailureTaskName.empty())
		{
			completion.Status = TaskExecutionStatus::Failed;
			completion.Result = m_firstFailure;
		}
	}

	if (completion.Status == TaskExecutionStatus::Invalid &&
	    m_observedCancellation.load(std::memory_order_acquire))
	{
		completion.Status = TaskExecutionStatus::Cancelled;
		completion.Result = TaskResult::Cancelled("Task execution contained cancellation.");
	}
	else if (completion.Status == TaskExecutionStatus::Invalid)
	{
		completion.Status = TaskExecutionStatus::Succeeded;
		completion.Result = TaskResult::Success();
	}

	return completion;
}

void TaskExecutor::Implementation::Runtime::ScheduledTaskExecution::Finish()
{
	bool expected = false;
	if (!m_finished.compare_exchange_strong(expected, true, std::memory_order_acq_rel))
	{
		return;
	}

	m_execution->Publish(BuildCompletion());
	m_owner.OnExecutionSettled();
}

#include "ScheduledTaskExecution.h"

#include "Profiling/TaskProfiler.h"

#include <utility>

TaskExecutor::Implementation::Runtime::ScheduledTaskExecution::ScheduledTaskExecution(
    Runtime& owner,
    std::shared_ptr<const TaskDetail::CompiledTaskGraphData> graph,
    TaskExecutionContext context,
    std::shared_ptr<TaskExecution::State> execution) :
	m_owner(owner), m_graph(std::move(graph)), m_context(std::move(context)), m_execution(std::move(execution)),
	m_generation(m_execution->Data.Generation), m_tasks(std::make_unique<ScheduledTaskState[]>(m_graph->Nodes.size())),
	m_taskResults(m_graph->Nodes.size()), m_settled(m_graph->Nodes.size(), false)
{
	for (std::uint32_t index = 0; index < m_graph->Nodes.size(); ++index)
	{
		m_tasks[index].RemainingPrerequisites.store(
		    static_cast<std::uint32_t>(m_graph->Nodes[index].Prerequisites.size()), std::memory_order_relaxed);
		m_tasks[index].UnfinishedCount.store(
		    1u + static_cast<std::uint32_t>(m_graph->Nodes[index].NestedChildren.size()), std::memory_order_relaxed);
		m_tasks[index].ParentBodyComplete.store(!m_graph->Nodes[index].Parent.has_value(), std::memory_order_relaxed);
	}
}

void TaskExecutor::Implementation::Runtime::ScheduledTaskExecution::Start()
{
	for (std::uint32_t dependent = 0; dependent < m_graph->Nodes.size(); ++dependent)
	{
		for (const std::uint32_t prerequisite : m_graph->Nodes[dependent].Prerequisites)
		{
			TaskDetail::RecordTaskDependency(m_generation, prerequisite, dependent);
		}
	}
	if (m_graph->Nodes.empty())
	{
		Finish();
		return;
	}
	for (std::uint32_t index = 0; index < m_graph->Nodes.size(); ++index)
	{
		TrySchedule(index, nullptr);
	}
}

void TaskExecutor::Implementation::Runtime::ScheduledTaskExecution::Execute(std::uint32_t index, TaskWorker& worker)
{
	ScheduledTaskState& task = m_tasks[index];
	const TaskDetail::CompiledTaskNode& node = m_graph->Nodes[index];
	const std::stop_token cancellation = m_execution->Cancellation.get_token();
	const auto taskStart = TaskDetail::BeginTaskProfile(node.Desc, m_generation, index, worker.LaneWorkerIndex);
	const bool blocked = task.BlockedByPrerequisite.load(std::memory_order_acquire) ||
	                     task.BlockedByParent.load(std::memory_order_acquire) || cancellation.stop_requested();
	TaskExecutionContext taskContext = m_context;
	TaskDetail::TaskExecutionContextAccess::Bind(taskContext, m_generation, node.Desc.Lane, cancellation);
	TaskResult bodyResult = blocked && node.Desc.CompletionPolicy == TaskCompletionPolicy::Normal
	                            ? TaskResult::Cancelled("Task execution was cancelled or a prerequisite did not succeed.")
	                            : TaskDetail::InvokeTask(node, taskContext);

	{
		std::lock_guard lock(m_resultMutex);
		m_taskResults[index] = bodyResult;
		if (bodyResult.Failed() && m_firstFailureTaskName.empty())
		{
			m_firstFailureTaskName = std::string(node.Desc.Name.Get());
			m_firstFailure = bodyResult;
		}
	}
	if (bodyResult.WasCancelled())
	{
		m_observedCancellation.store(true, std::memory_order_release);
	}
	TaskDetail::EndTaskProfile(node.Desc, m_generation, index, worker.LaneWorkerIndex, bodyResult, taskStart);

	for (const std::uint32_t childIndex : node.NestedChildren)
	{
		ScheduledTaskState& child = m_tasks[childIndex];
		if (!bodyResult.Succeeded())
		{
			child.BlockedByParent.store(true, std::memory_order_release);
		}
		child.ParentBodyComplete.store(true, std::memory_order_release);
		TrySchedule(childIndex, &worker);
	}
	ReleaseUnfinished(index, &worker);
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

void TaskExecutor::Implementation::Runtime::ScheduledTaskExecution::Finish()
{
	bool expected = false;
	if (!m_finished.compare_exchange_strong(expected, true, std::memory_order_acq_rel))
	{
		return;
	}

	TaskDetail::CompletedTaskExecution completed;
	completed.Generation = m_generation;
	completed.BuilderIdentity = m_graph->BuilderIdentity;
	completed.BuilderGeneration = m_graph->BuilderGeneration;
	completed.SettledTaskCount = m_settledTaskCount.load(std::memory_order_acquire);
	{
		std::lock_guard lock(m_resultMutex);
		completed.TaskResults = m_taskResults;
		completed.Settled = m_settled;
		completed.FirstFailureTaskName = m_firstFailureTaskName;
		if (!m_firstFailureTaskName.empty())
		{
			completed.Status = TaskExecutionStatus::Failed;
			completed.Result = m_firstFailure;
		}
	}
	if (completed.Status == TaskExecutionStatus::Invalid && m_observedCancellation.load(std::memory_order_acquire))
	{
		completed.Status = TaskExecutionStatus::Cancelled;
		completed.Result = TaskResult::Cancelled("Task execution contained cancellation.");
	}
	else if (completed.Status == TaskExecutionStatus::Invalid)
	{
		completed.Status = TaskExecutionStatus::Succeeded;
		completed.Result = TaskResult::Success();
	}
	m_execution->Publish(std::move(completed));
	m_owner.OnExecutionSettled();
}

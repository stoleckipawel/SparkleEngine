#include "TaskExecutorImplementation.h"

#include "ScheduledTaskExecution.h"
#include "TaskExecutorRuntime.h"

#include <algorithm>
#include <stdexcept>
#include <utility>

class TaskExecutorMechanism final
{
  public:
	static TaskDetail::CompletedTaskExecution RejectedExecution(std::uint64_t generation, std::string_view reason)
	{
		TaskDetail::CompletedTaskExecution execution;
		execution.Generation = generation;
		execution.Status = TaskExecutionStatus::Rejected;
		execution.Result = TaskResult::Failure(reason);
		return execution;
	}
};

TaskExecutor::Implementation::Runtime::Runtime(TaskExecutorConfig config) : m_config(config)
{
	const std::uint32_t totalWorkers =
	    config.FrameCriticalWorkerCount + config.BackgroundWorkerCount + config.BlockingIoWorkerCount;
	const bool invalidSerialMix = config.FrameCriticalWorkerCount == 0 &&
	                              (config.BackgroundWorkerCount != 0 || config.BlockingIoWorkerCount != 0);
	if (totalWorkers > MaximumWorkerCount || invalidSerialMix || m_config.MaximumTasksPerExecution == 0 ||
	    m_config.MaximumTasksPerExecution > TaskGraphLimits::HardMaximumTasks ||
	    m_config.MaximumEdgesPerExecution > TaskGraphLimits::HardMaximumEdges || m_config.MaximumActiveExecutions == 0)
	{
		throw std::invalid_argument("TaskExecutorConfig contains an unsupported lane worker count or capacity.");
	}

	m_workers.reserve(totalWorkers);
	m_executions.reserve(m_config.MaximumActiveExecutions);
	AddWorkers(TaskLane::FrameCritical, config.FrameCriticalWorkerCount);
	AddWorkers(TaskLane::Background, config.BackgroundWorkerCount);
	AddWorkers(TaskLane::BlockingIo, config.BlockingIoWorkerCount);
	try
	{
		for (const auto& worker : m_workers)
		{
			worker->Thread = std::thread([this, worker = worker.get()] { WorkerMain(*worker); });
		}
	}
	catch (...)
	{
		RequestWorkerStop();
		JoinWorkers();
		throw;
	}
}

TaskExecutor::Implementation::Runtime::~Runtime()
{
	Shutdown(TaskExecutorShutdownMode::Drain);
}

std::shared_ptr<TaskExecution::State> TaskExecutor::Implementation::Runtime::Launch(
    const CompiledTaskGraph& graph,
    TaskExecutionContext context,
    const std::shared_ptr<TaskScope::State>& scope)
{
	const std::uint64_t generation = m_nextExecutionGeneration.fetch_add(1, std::memory_order_relaxed);
	auto execution = std::make_shared<TaskExecution::State>(generation);
	execution->JoinThread = scope ? scope->OwnerThread : std::this_thread::get_id();
	execution->ExecutorIdentity = this;
	TaskDetail::TaskExecutionContextAccess::Bind(
	    context, generation, TaskLane::FrameCritical, execution->Cancellation.get_token());

	if (TaskDetail::IsExecutorWorker(this))
	{
		execution->Publish(TaskExecutorMechanism::RejectedExecution(
		    generation, "A task worker cannot submit work to its own executor; use graph dependencies or nested tasks."));
		return execution;
	}
	if (scope && context.HasUserData() && !context.HasOwnedUserData())
	{
		execution->Publish(TaskExecutorMechanism::RejectedExecution(
		    generation, "Scoped asynchronous launch requires owned or empty TaskExecutionContext data."));
		return execution;
	}
	if (!graph.IsValid())
	{
		execution->Publish(TaskExecutorMechanism::RejectedExecution(generation, graph.GetError().Message));
		return execution;
	}
	if (graph.GetTaskCount() > m_config.MaximumTasksPerExecution || graph.GetEdgeCount() > m_config.MaximumEdgesPerExecution)
	{
		execution->Publish(TaskExecutorMechanism::RejectedExecution(generation, "Compiled task graph exceeds this executor's bounded execution capacity."));
		return execution;
	}
	if (!m_workers.empty())
	{
		for (const auto& node : graph.m_data->Nodes)
		{
			if (GetWorkerCount(node.Desc.Lane) == 0)
			{
				execution->Publish(TaskExecutorMechanism::RejectedExecution(generation, "Compiled task graph uses a lane with no configured workers."));
				return execution;
			}
		}
	}

	auto scheduledExecution = m_workers.empty() ? std::shared_ptr<ScheduledTaskExecution>{}
	                                            : std::make_shared<ScheduledTaskExecution>(
	                                                  *this, graph.m_data, std::move(context), execution);
	if (scope && !scope->RegisterExecution(execution))
	{
		execution->Publish(TaskExecutorMechanism::RejectedExecution(generation, "TaskScope is closed, settled, or used from a non-owner thread."));
		return execution;
	}

	{
		std::lock_guard lock(m_stateMutex);
		std::erase_if(m_executions, [](const std::weak_ptr<TaskExecution::State>& item) { return item.expired(); });
		if (m_lifecycle != LifecycleState::Accepting)
		{
			execution->Publish(TaskExecutorMechanism::RejectedExecution(generation, "Task executor is no longer accepting submissions."));
			return execution;
		}
		if (m_activeExecutions >= m_config.MaximumActiveExecutions)
		{
			execution->Publish(TaskExecutorMechanism::RejectedExecution(generation, "Task executor reached its active-execution capacity."));
			return execution;
		}
		++m_activeExecutions;
		m_executions.emplace_back(execution);
	}

	if (m_workers.empty())
	{
		try
		{
			auto completed = TaskDetail::ExecuteSerial(
			    *graph.m_data, context, generation, execution->Cancellation.get_token());
			execution->Publish(std::move(completed));
			OnExecutionSettled();
		}
		catch (...)
		{
			OnExecutionSettled();
			throw;
		}
	}
	else
	{
		scheduledExecution->Start();
	}
	return execution;
}

void TaskExecutor::Implementation::Runtime::OnExecutionSettled()
{
	std::lock_guard lock(m_stateMutex);
	--m_activeExecutions;
	m_stateCondition.notify_all();
}

bool TaskExecutor::Implementation::Runtime::Shutdown(TaskExecutorShutdownMode mode) noexcept
{
	if (TaskDetail::IsExecutorWorker(this))
	{
		return false;
	}
	std::unique_lock shutdownLock(m_shutdownMutex);
	std::vector<std::shared_ptr<TaskExecution::State>> executionsToCancel;
	{
		std::unique_lock lock(m_stateMutex);
		if (m_lifecycle == LifecycleState::Stopped)
		{
			return true;
		}
		if (m_lifecycle == LifecycleState::Accepting)
		{
			m_lifecycle = mode == TaskExecutorShutdownMode::Cancel ? LifecycleState::Cancelling : LifecycleState::Draining;
		}
		if (m_lifecycle == LifecycleState::Cancelling)
		{
			for (auto& item : m_executions)
			{
				if (auto execution = item.lock())
				{
					executionsToCancel.push_back(std::move(execution));
				}
			}
		}
	}
	for (const auto& execution : executionsToCancel)
	{
		execution->RequestCancellation();
	}
	{
		std::unique_lock lock(m_stateMutex);
		m_stateCondition.wait(lock, [this] { return m_activeExecutions == 0; });
		m_lifecycle = LifecycleState::Stopping;
	}
	RequestWorkerStop();
	JoinWorkers();
	{
		std::lock_guard lock(m_stateMutex);
		m_lifecycle = LifecycleState::Stopped;
		m_executions.clear();
	}
	m_stateCondition.notify_all();
	return true;
}

TaskExecutor::Implementation::Implementation(TaskExecutorConfig config) :
	m_runtime(std::make_unique<Runtime>(config))
{
}

TaskExecutor::Implementation::~Implementation() = default;

std::shared_ptr<TaskExecution::State> TaskExecutor::Implementation::Launch(
    const CompiledTaskGraph& graph,
    TaskExecutionContext context,
    TaskScope* scope)
{
	return m_runtime->Launch(graph, std::move(context), scope != nullptr ? scope->m_state : nullptr);
}

bool TaskExecutor::Implementation::Shutdown(TaskExecutorShutdownMode mode) noexcept
{
	return m_runtime->Shutdown(mode);
}

std::uint32_t TaskExecutor::Implementation::GetWorkerCount(TaskLane lane) const noexcept
{
	return m_runtime->GetWorkerCount(lane);
}

const TaskExecutorConfig& TaskExecutor::Implementation::GetConfig() const noexcept
{
	return m_runtime->GetConfig();
}

#include "TaskExecutorImplementation.h"

#include "Execution/SerialTaskExecution.h"
#include "ScheduledTaskExecution.h"
#include "TaskExecutorRuntime.h"
#include "TaskWorkerContext.h"

#include "Core/Public/Diagnostics/Verify.h"

#include <algorithm>
#include <cassert>
#include <stdexcept>
#include <utility>

static const auto g_taskExecutorLogger = Logging::GetOrCreateLogger("Tasks.Executor");

void TaskExecutor::Implementation::Runtime::ValidateConfiguration(const TaskExecutorConfig& config)
{
	const std::uint32_t totalWorkers = config.FrameCriticalWorkerCount + config.BackgroundWorkerCount + config.BlockingIoWorkerCount;
	const bool invalidSerialMix =
	    config.FrameCriticalWorkerCount == 0 && (config.BackgroundWorkerCount != 0 || config.BlockingIoWorkerCount != 0);
	const bool invalidCapacity = config.MaximumTasksPerExecution == 0 || config.MaximumTasksPerExecution > TaskGraphLimits::HardMaximumTasks
	    || config.MaximumEdgesPerExecution > TaskGraphLimits::HardMaximumEdges || config.MaximumActiveExecutions == 0;
	if (totalWorkers > MaximumWorkerCount || invalidSerialMix || invalidCapacity)
	{
		throw std::invalid_argument("TaskExecutorConfig contains an unsupported lane worker count or capacity.");
	}
}

bool TaskExecutor::Implementation::Runtime::RejectExecution(
    const std::shared_ptr<TaskExecution::State>& execution,
    std::uint64_t generation,
    std::string_view reason)
{
	TaskExecutionCompletion completion;
	completion.Generation = generation;
	completion.Status = TaskExecutionStatus::Rejected;
	completion.Result = TaskResult::Failure(reason);
	execution->Publish(std::move(completion));
	return false;
}

TaskExecutor::Implementation::Runtime::Runtime(TaskExecutorConfig config) :
    m_config(config)
{
	ValidateConfiguration(config);

	const std::uint32_t totalWorkers = config.FrameCriticalWorkerCount + config.BackgroundWorkerCount + config.BlockingIoWorkerCount;
	m_workers.reserve(totalWorkers);
	m_executions.reserve(m_config.MaximumActiveExecutions);

	AddWorkers(TaskLane::FrameCritical, config.FrameCriticalWorkerCount);
	AddWorkers(TaskLane::Background, config.BackgroundWorkerCount);
	AddWorkers(TaskLane::BlockingIo, config.BlockingIoWorkerCount);
	StartWorkers();
}

void TaskExecutor::Implementation::Runtime::StartWorkers()
{
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

std::shared_ptr<TaskExecution::State> TaskExecutor::Implementation::Runtime::CreateExecution(
    std::uint64_t generation,
    const std::shared_ptr<TaskScope::State>& scope) const
{
	auto execution = std::make_shared<TaskExecution::State>(generation);
	execution->JoinThread = scope ? scope->OwnerThread : std::this_thread::get_id();
	execution->ExecutorIdentity = this;
	return execution;
}

bool TaskExecutor::Implementation::Runtime::ValidateWorkerLanes(
    const CompiledTaskGraph& graph,
    const std::shared_ptr<TaskExecution::State>& execution,
    std::uint64_t generation) const
{
	if (m_workers.empty())
	{
		return true;
	}

	for (const TaskGraphNode& node : graph.m_data->Nodes)
	{
		if (GetWorkerCount(node.Desc.Lane) == 0)
		{
			return RejectExecution(execution, generation, "Compiled task graph uses a lane with no configured workers.");
		}
	}

	return true;
}

bool TaskExecutor::Implementation::Runtime::ValidateLaunchRequest(
    const CompiledTaskGraph& graph,
    const TaskExecutionContext& context,
    const std::shared_ptr<TaskScope::State>& scope,
    const std::shared_ptr<TaskExecution::State>& execution,
    std::uint64_t generation) const
{
	if (TaskWorkerContext::IsWorkerFor(this))
	{
		return RejectExecution(
		    execution,
		    generation,
		    "A task worker cannot submit work to its own executor; use graph dependencies or nested tasks.");
	}
	if (scope && context.HasUserData() && !context.HasOwnedUserData())
	{
		return RejectExecution(execution, generation, "Scoped asynchronous launch received non-owned TaskExecutionContext data.");
	}
	if (!graph.IsValid())
	{
		return RejectExecution(execution, generation, graph.GetError().Message);
	}
	if (graph.GetTaskCount() > m_config.MaximumTasksPerExecution || graph.GetEdgeCount() > m_config.MaximumEdgesPerExecution)
	{
		return RejectExecution(execution, generation, "Compiled task graph exceeds this executor's bounded execution capacity.");
	}

	return ValidateWorkerLanes(graph, execution, generation);
}

bool TaskExecutor::Implementation::Runtime::RegisterScopedExecution(
    const std::shared_ptr<TaskScope::State>& scope,
    const std::shared_ptr<TaskExecution::State>& execution,
    std::uint64_t generation) const
{
	if (!scope || scope->RegisterExecution(execution))
	{
		return true;
	}

	return RejectExecution(execution, generation, "TaskScope is closed, settled, or used from a non-owner thread.");
}

bool TaskExecutor::Implementation::Runtime::AdmitExecution(const std::shared_ptr<TaskExecution::State>& execution, std::uint64_t generation)
{
	std::lock_guard lock(m_stateMutex);
	std::erase_if(m_executions, [](const std::weak_ptr<TaskExecution::State>& item) { return item.expired(); });

	if (m_lifecycle != LifecycleState::Accepting)
	{
		return RejectExecution(execution, generation, "Task executor is no longer accepting submissions.");
	}
	if (m_activeExecutions >= m_config.MaximumActiveExecutions)
	{
		return RejectExecution(execution, generation, "Task executor reached its active-execution capacity.");
	}

	++m_activeExecutions;
	m_executions.emplace_back(execution);
	return true;
}

void TaskExecutor::Implementation::Runtime::ExecuteSerial(
    const CompiledTaskGraph& graph,
    TaskExecutionContext& context,
    const std::shared_ptr<TaskExecution::State>& execution)
{
	try
	{
		TaskExecutionCompletion completion =
		    SerialTaskExecution::Execute(*graph.m_data, context, execution->Data.Generation, execution->Cancellation.get_token());
		execution->Publish(std::move(completion));
		OnExecutionSettled();
	}
	catch (...)
	{
		Diagnostics::Fatal(
		    g_taskExecutorLogger,
		    __FILE__,
		    __LINE__,
		    "Serial task execution failed after admission and could not publish completion.");
	}
}

void TaskExecutor::Implementation::Runtime::StartExecution(
    const CompiledTaskGraph& graph,
    TaskExecutionContext context,
    const std::shared_ptr<TaskExecution::State>& execution)
{
	if (m_workers.empty())
	{
		ExecuteSerial(graph, context, execution);
		return;
	}

	try
	{
		auto scheduledExecution = std::make_shared<ScheduledTaskExecution>(*this, graph.m_data, std::move(context), execution);
		scheduledExecution->Start();
	}
	catch (...)
	{
		Diagnostics::Fatal(
		    g_taskExecutorLogger,
		    __FILE__,
		    __LINE__,
		    "Threaded task execution failed after admission and could not safely roll back publication.");
	}
}

std::shared_ptr<TaskExecution::State> TaskExecutor::Implementation::Runtime::Launch(
    const CompiledTaskGraph& graph,
    TaskExecutionContext context,
    const std::shared_ptr<TaskScope::State>& scope)
{
	const std::uint64_t generation = m_nextExecutionGeneration.fetch_add(1, std::memory_order_relaxed);
	auto execution = CreateExecution(generation, scope);

	TaskExecutionContextBinding::Bind(context, generation, TaskLane::FrameCritical, execution->Cancellation.get_token());

	if (!ValidateLaunchRequest(graph, context, scope, execution, generation))
	{
		return execution;
	}
	if (!RegisterScopedExecution(scope, execution, generation))
	{
		return execution;
	}
	if (!AdmitExecution(execution, generation))
	{
		return execution;
	}

	StartExecution(graph, std::move(context), execution);
	return execution;
}

void TaskExecutor::Implementation::Runtime::OnExecutionSettled()
{
	{
		std::lock_guard lock(m_stateMutex);
		assert(m_activeExecutions > 0 && "Task executor active execution count underflowed.");
		--m_activeExecutions;
	}
	m_stateCondition.notify_all();
}

bool TaskExecutor::Implementation::Runtime::Shutdown(TaskExecutorShutdownMode mode) noexcept
{
	if (TaskWorkerContext::IsWorkerFor(this))
	{
		return false;
	}

	std::unique_lock shutdownLock(m_shutdownMutex);
	const std::vector<std::shared_ptr<TaskExecution::State>> executionsToCancel = BeginShutdown(mode);
	RequestCancellation(executionsToCancel);
	WaitForActiveExecutions();
	FinishShutdown();
	return true;
}

std::vector<std::shared_ptr<TaskExecution::State>> TaskExecutor::Implementation::Runtime::BeginShutdown(TaskExecutorShutdownMode mode)
{
	std::vector<std::shared_ptr<TaskExecution::State>> executionsToCancel;
	std::unique_lock lock(m_stateMutex);
	if (m_lifecycle == LifecycleState::Stopped)
	{
		return executionsToCancel;
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

	return executionsToCancel;
}

void TaskExecutor::Implementation::Runtime::RequestCancellation(std::span<const std::shared_ptr<TaskExecution::State>> executions) noexcept
{
	for (const auto& execution : executions)
	{
		execution->RequestCancellation();
	}
}

void TaskExecutor::Implementation::Runtime::WaitForActiveExecutions()
{
	std::unique_lock lock(m_stateMutex);
	m_stateCondition.wait(lock, [this] { return m_activeExecutions == 0; });
	m_lifecycle = LifecycleState::Stopping;
}

void TaskExecutor::Implementation::Runtime::FinishShutdown()
{
	RequestWorkerStop();
	JoinWorkers();

	{
		std::lock_guard lock(m_stateMutex);
		m_lifecycle = LifecycleState::Stopped;
		m_executions.clear();
	}

	m_stateCondition.notify_all();
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

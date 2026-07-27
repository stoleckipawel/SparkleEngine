#include "TaskExecutor.h"

#include "TaskExecutorImplementation.h"
#include "Execution/TaskExecutionState.h"

#include <utility>

TaskExecutor::TaskExecutor(TaskExecutorConfig config) : m_implementation(std::make_unique<Implementation>(config)) {}

TaskExecutor::~TaskExecutor() = default;

TaskExecution TaskExecutor::Submit(const CompiledTaskGraph& graph, TaskExecutionContext& context)
{
	auto state = m_implementation->Launch(graph, context, nullptr);
	{
		std::unique_lock lock(state->Mutex);
		state->Condition.wait(lock, [&state] { return state->Settled; });
	}
	return TaskExecution(std::move(state));
}

TaskExecution TaskExecutor::Submit(TaskDesc desc, TaskFunction function, TaskExecutionContext& context)
{
	const TaskExecutorConfig& config = m_implementation->GetConfig();
	TaskGraphBuilder builder(TaskGraphLimits{
	    .MaximumTasks = config.MaximumTasksPerExecution,
	    .MaximumEdges = config.MaximumEdgesPerExecution});
	builder.Add(std::move(desc), std::move(function));
	return Submit(builder.Compile(), context);
}

TaskExecution TaskExecutor::Launch(TaskScope& scope, const CompiledTaskGraph& graph, TaskExecutionContext context)
{
	return TaskExecution(m_implementation->Launch(graph, std::move(context), &scope));
}

TaskExecution TaskExecutor::Launch(TaskScope& scope, TaskDesc desc, TaskFunction function, TaskExecutionContext context)
{
	const TaskExecutorConfig& config = m_implementation->GetConfig();
	TaskGraphBuilder builder(TaskGraphLimits{
	    .MaximumTasks = config.MaximumTasksPerExecution,
	    .MaximumEdges = config.MaximumEdgesPerExecution});
	builder.Add(std::move(desc), std::move(function));
	return Launch(scope, builder.Compile(), std::move(context));
}

bool TaskExecutor::Shutdown(TaskExecutorShutdownMode mode) noexcept
{
	return m_implementation->Shutdown(mode);
}

std::uint32_t TaskExecutor::GetWorkerCount(TaskLane lane) const noexcept
{
	return m_implementation->GetWorkerCount(lane);
}

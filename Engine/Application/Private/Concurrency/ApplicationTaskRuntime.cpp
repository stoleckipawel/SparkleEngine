#include "PCH.h"

#include "Concurrency/ApplicationTaskRuntime.h"

#include "Concurrency/TaskRuntimeCVars.h"
#include "Tasks/Public/TaskExecutor.h"
#include "Tasks/Public/TaskScope.h"

#include <chrono>

TaskExecutorConfig ApplicationTaskRuntime::BuildExecutorConfig() noexcept
{
	if (TaskRuntimeCVars::UseSerialExecution())
	{
		return {};
	}

	return TaskExecutorConfig{
	    .FrameCriticalWorkerCount = TaskRuntimeCVars::ResolveFrameCriticalWorkerCount(),
	    .BackgroundWorkerCount = TaskRuntimeCVars::ResolveBackgroundWorkerCount(),
	    .BlockingIoWorkerCount = TaskRuntimeCVars::ResolveBlockingIoWorkerCount(),
	    .MaximumTasksPerExecution = 1'024,
	    .MaximumEdgesPerExecution = 4'096,
	    .MaximumActiveExecutions = 64};
}

ApplicationTaskRuntime::ApplicationTaskRuntime() :
    m_executor(std::make_unique<TaskExecutor>(BuildExecutorConfig())),
    m_applicationScope(std::make_unique<TaskScope>(TaskScopeDesc{TaskScopeKind::Application, "Application"}))
{
}

ApplicationTaskRuntime::~ApplicationTaskRuntime()
{
	m_applicationScope->Cancel();
	m_executor->Shutdown(TaskExecutorShutdownMode::Cancel);
	m_applicationScope->JoinFor(std::chrono::milliseconds::zero());
}

TaskExecutor& ApplicationTaskRuntime::GetExecutor() noexcept
{
	return *m_executor;
}

TaskScope& ApplicationTaskRuntime::GetApplicationScope() noexcept
{
	return *m_applicationScope;
}

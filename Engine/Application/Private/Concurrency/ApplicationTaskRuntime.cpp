#include "PCH.h"

#include "Concurrency/ApplicationTaskRuntime.h"

#include "Concurrency/TaskRuntimeCVars.h"
#include "Tasks/Public/TaskExecutor.h"
#include "Tasks/Public/TaskScope.h"

#include <chrono>

class ApplicationTaskRuntimeOperations final
{
  public:
	static TaskExecutorConfig BuildTaskExecutorConfig() noexcept
	{
		if (TaskRuntimeCVars::UseSerialExecution())
		{
			return {};
		}

		const std::uint32_t configuredWorkers = TaskRuntimeCVars::ResolveWorkerCount();
		const std::uint32_t cpuWorkers = configuredWorkers == 0 ? 1u : configuredWorkers;
		return TaskExecutorConfig{
		    .FrameCriticalWorkerCount = cpuWorkers,
		    .BackgroundWorkerCount = cpuWorkers,
		    .BlockingIoWorkerCount = 1,
		    .MaximumTasksPerExecution = 1'024,
		    .MaximumEdgesPerExecution = 4'096,
		    .MaximumActiveExecutions = 64};
	}
};

ApplicationTaskRuntime::ApplicationTaskRuntime() :
    m_executor(std::make_unique<TaskExecutor>(ApplicationTaskRuntimeOperations::BuildTaskExecutorConfig())),
    m_applicationScope(std::make_unique<TaskScope>(TaskScopeDesc{TaskScopeKind::Application, "Application"}))
{
}

ApplicationTaskRuntime::~ApplicationTaskRuntime()
{
	m_applicationScope->Cancel();
	m_executor->Shutdown(TaskExecutorShutdownMode::Cancel);
	m_applicationScope->JoinFor(std::chrono::milliseconds::zero());
}

TaskExecutor& ApplicationTaskRuntime::GetExecutor() noexcept { return *m_executor; }

TaskScope& ApplicationTaskRuntime::GetApplicationScope() noexcept { return *m_applicationScope; }

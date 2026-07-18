#include "LauncherOperationService.h"

#include "LauncherBackend.h"
#include "LauncherOperationExecution.h"
#include "Tasks/Public/TaskExecutor.h"
#include "Tasks/Public/TaskScope.h"

#include <utility>

namespace SparkleLauncher
{
	struct LauncherOperationService::Implementation final
	{
		explicit Implementation(ProcessRunnerFactory processRunnerFactory) : ProcessRunnerFactoryValue(std::move(processRunnerFactory)) {}

		ProcessRunnerFactory ProcessRunnerFactoryValue;
		TaskExecutor Executor{TaskExecutorConfig{
		    .FrameCriticalWorkerCount = 1,
		    .BackgroundWorkerCount = 1,
		    .BlockingIoWorkerCount = 2,
		    .MaximumActiveExecutions = 16}};
		TaskScope Scope{TaskScopeDesc{TaskScopeKind::Application, "Launcher operations"}};
	};

	LauncherOperationService::LauncherOperationService(ProcessRunnerFactory processRunnerFactory) :
	    m_implementation(std::make_unique<Implementation>(std::move(processRunnerFactory)))
	{
	}

	LauncherOperationService::~LauncherOperationService()
	{
		m_implementation->Scope.Cancel();
		m_implementation->Executor.Shutdown(TaskExecutorShutdownMode::Cancel);
		m_implementation->Scope.JoinFor(std::chrono::milliseconds::zero());
	}

	void LauncherOperationService::Launch(
	    LauncherOperationCategory category,
	    LauncherOperationRequest request,
	    std::string title,
	    OutputCallback outputCallback,
	    CompletionCallback completionCallback)
	{
		ProcessRunnerFactory processRunnerFactory = m_implementation->ProcessRunnerFactoryValue;
		m_implementation->Executor.Launch(
		    m_implementation->Scope,
		    TaskDesc{TaskName("Launcher operation"), TaskLane::BlockingIo},
		    [category,
		     request = std::move(request),
		     title = std::move(title),
		     processRunnerFactory = std::move(processRunnerFactory),
		     outputCallback = std::move(outputCallback),
		     completionCallback = std::move(completionCallback)](TaskExecutionContext& context)
		    {
			    const std::string operationId = request.OperationId.toStdString();
			    std::unique_ptr<IProcessRunner> processRunner = processRunnerFactory();
			    if (!processRunner)
			    {
				    OperationRecord record = MakeOperationRecord(operationId, title);
				    record.Status = OperationStatus::Failed;
				    record.FailureSummary = "No process runner is available for this launcher operation.";
				    completionCallback(std::move(record));
				    return TaskResult::Failure("No process runner is available.");
			    }
			    OperationRecord record =
			        ExecuteLauncherOperation(category, operationId, title, request, *processRunner, context, outputCallback);
			    completionCallback(std::move(record));
			    return context.IsCancellationRequested() ? TaskResult::Cancelled("Launcher operation was cancelled.")
			                                             : TaskResult::Success();
		    });
	}
}

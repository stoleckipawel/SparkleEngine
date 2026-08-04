#include "LauncherOperationService.h"

#include "LauncherOperationExecution.h"
#include "Tasks/Public/TaskExecutor.h"
#include "Tasks/Public/TaskScope.h"

#include <map>
#include <stdexcept>
#include <utility>

namespace SparkleLauncher
{
	struct LauncherOperationService::Implementation final
	{
		explicit Implementation(ProcessRunnerFactory processRunnerFactory) :
		    m_processRunnerFactory(std::move(processRunnerFactory))
		{
		}
		~Implementation()
		{
			m_rootScope.Cancel();
			m_executor.Shutdown(TaskExecutorShutdownMode::Cancel);
			for (const auto& operation : m_operationScopes)
			{
				operation.second->JoinFor(std::chrono::milliseconds::zero());
			}
			m_rootScope.JoinFor(std::chrono::milliseconds::zero());
		}

		void ReapSettledOperations()
		{
			for (auto operation = m_operationScopes.begin(); operation != m_operationScopes.end();)
			{
				// JoinFor closes the per-run scope. A scope does not become settled merely
				// because its only execution finished; the owner must close it explicitly.
				if (!operation->second->JoinFor(std::chrono::milliseconds::zero()))
				{
					++operation;
					continue;
				}

				operation = m_operationScopes.erase(operation);
			}
		}

		ProcessRunnerFactory m_processRunnerFactory;
		TaskExecutor m_executor{TaskExecutorConfig{
		    .FrameCriticalWorkerCount = 1,
		    .BackgroundWorkerCount = 1,
		    .BlockingIoWorkerCount = 2,
		    .MaximumActiveExecutions = 16}};
		TaskScope m_rootScope{TaskScopeDesc{TaskScopeKind::Application, "Launcher operations"}};
		std::map<std::string, std::unique_ptr<TaskScope>, std::less<>> m_operationScopes;
	};

	LauncherOperationService::LauncherOperationService(ProcessRunnerFactory processRunnerFactory) :
	    m_implementation(std::make_unique<Implementation>(std::move(processRunnerFactory)))
	{
	}

	LauncherOperationService::~LauncherOperationService() = default;

	void LauncherOperationService::Launch(
	    LauncherOperationCategory category,
	    LauncherOperationRequest request,
	    std::string title,
	    OutputCallback outputCallback,
	    CompletionCallback completionCallback)
	{
		m_implementation->ReapSettledOperations();
		const std::string runId = request.RunId.isEmpty() ? request.OperationId.toStdString() : request.RunId.toStdString();
		if (runId.empty() || m_implementation->m_operationScopes.contains(runId))
		{
			throw std::logic_error("Launcher operation run identity is empty or already active.");
		}

		auto operationScope = std::make_unique<TaskScope>(
		    TaskScopeDesc{TaskScopeKind::ToolInvocation, "Launcher operation " + runId},
		    &m_implementation->m_rootScope);
		TaskScope& scope = *operationScope;
		m_implementation->m_operationScopes.emplace(runId, std::move(operationScope));
		ProcessRunnerFactory processRunnerFactory = m_implementation->m_processRunnerFactory;
		try
		{
			m_implementation->m_executor.Launch(
			    scope,
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
				        ExecuteLauncherOperation(category, operationId, request, *processRunner, context, outputCallback);
				    completionCallback(std::move(record));
				    return context.IsCancellationRequested() ? TaskResult::Cancelled("Launcher operation was cancelled.")
				                                             : TaskResult::Success();
			    });
		}
		catch (...)
		{
			scope.JoinFor(std::chrono::milliseconds::zero());
			m_implementation->m_operationScopes.erase(runId);
			throw;
		}
	}

	bool LauncherOperationService::Cancel(std::string_view runId) noexcept
	{
		const auto operation = m_implementation->m_operationScopes.find(runId);
		if (operation == m_implementation->m_operationScopes.end() || operation->second->IsSettled())
		{
			return false;
		}

		operation->second->Cancel();
		return true;
	}
}

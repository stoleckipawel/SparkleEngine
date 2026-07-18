#include "LauncherOperationExecution.h"

#include "LauncherBackend.h"
#include "LauncherOperationRequestMapping.h"
#include "SparkleLauncher/BuildWorkspaceOperations.h"
#include "SparkleLauncher/CookOperations.h"
#include "SparkleLauncher/LaunchOperations.h"
#include "SparkleLauncher/MaintenanceOperations.h"
#include "Tasks/Public/TaskExecutionContext.h"

#include <memory>
#include <utility>

namespace SparkleLauncher
{
	class TaskContextProcessRunner final : public IProcessRunner
	{
	  public:
		TaskContextProcessRunner(IProcessRunner& runner, TaskExecutionContext& context) : m_runner(runner), m_context(context) {}

		ProcessResult Run(const ProcessRequest& request) override
		{
			ProcessRequest scopedRequest = request;
			scopedRequest.Cancellation = m_context.GetCancellationToken();
			return m_runner.Run(scopedRequest);
		}

	  private:
		IProcessRunner& m_runner;
		TaskExecutionContext& m_context;
	};

	OperationRecord ExecuteLauncherOperation(
	    LauncherOperationCategory category,
	    std::string operationId,
	    std::string title,
	    const LauncherOperationRequest& request,
	    IProcessRunner& processRunner,
	    TaskExecutionContext& context,
	    const ProcessOutputCallback& outputCallback)
	{
		TaskContextProcessRunner taskProcessRunner(processRunner, context);
		OperationRecord record = MakeOperationRecord(operationId, title);
		switch (category)
		{
			case LauncherOperationCategory::Workspace:
			{
				BuildWorkspaceOperationPlan plan =
				    PlanBuildWorkspaceOperation(operationId, LauncherOperationRequestMapping::BuildWorkspace(request));
				if (plan.CanRun)
					return RunBuildWorkspaceOperationPlan(std::move(plan), taskProcessRunner, outputCallback);
				record = plan.Operation;
				record.FailureSummary = plan.ReadinessMessages.empty() ? "Operation readiness failed." : plan.ReadinessMessages.back();
				break;
			}
			case LauncherOperationCategory::Cooking:
			{
				CookOperationPlan plan = PlanCookOperation(operationId, LauncherOperationRequestMapping::Cook(request));
				if (plan.CanRun)
					return RunCookOperationPlan(std::move(plan), taskProcessRunner, outputCallback);
				record = plan.Operation;
				record.FailureSummary = plan.ReadinessMessages.empty() ? "Operation readiness failed." : plan.ReadinessMessages.back();
				break;
			}
			case LauncherOperationCategory::Maintenance:
			{
				MaintenanceOperationPlan plan =
				    PlanMaintenanceOperation(operationId, LauncherOperationRequestMapping::Maintenance(request));
				if (plan.CanRun)
					return RunMaintenanceOperationPlan(std::move(plan), taskProcessRunner, outputCallback);
				record = plan.Operation;
				record.FailureSummary = plan.ReadinessMessages.empty() ? "Operation readiness failed." : plan.ReadinessMessages.back();
				break;
			}
			case LauncherOperationCategory::Launch:
			{
				LaunchOperationPlan plan = PlanLaunchOperation(operationId, LauncherOperationRequestMapping::Launch(request));
				if (plan.CanRun)
					return RunLaunchOperationPlan(std::move(plan), taskProcessRunner, outputCallback);
				record = plan.Operation;
				record.FailureSummary = plan.ReadinessMessages.empty() ? "Operation readiness failed." : plan.ReadinessMessages.back();
				break;
			}
		}
		record.Status = OperationStatus::Skipped;
		return record;
	}
}

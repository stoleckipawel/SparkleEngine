#include "LauncherOperationExecution.h"

#include "LauncherOperationRequestMapping.h"
#include "Tasks/Public/TaskExecutionContext.h"

#include <memory>
#include <stdexcept>
#include <type_traits>
#include <utility>

namespace SparkleLauncher
{
	class TaskContextProcessRunner final : public IProcessRunner
	{
	public:
		explicit TaskContextProcessRunner(IProcessRunner& runner, TaskExecutionContext& context) :
		    m_runner(runner),
		    m_context(context)
		{
		}

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

	LauncherOperationPlan PlanLauncherOperation(
	    LauncherOperationCategory category,
	    std::string_view operationId,
	    const LauncherOperationRequest& request)
	{
		switch (category)
		{
			case LauncherOperationCategory::Workspace:
				return PlanBuildWorkspaceOperation(operationId, LauncherOperationRequestMapping::BuildWorkspace(request));
			case LauncherOperationCategory::Cooking:
				return PlanCookOperation(operationId, LauncherOperationRequestMapping::Cook(request));
			case LauncherOperationCategory::Maintenance:
				return PlanMaintenanceOperation(operationId, LauncherOperationRequestMapping::Maintenance(request));
			case LauncherOperationCategory::Launch:
				return PlanLaunchOperation(operationId, LauncherOperationRequestMapping::Launch(request));
		}

		throw std::logic_error("Unknown launcher operation category.");
	}

	OperationRecord ExecuteLauncherOperation(
	    LauncherOperationCategory category,
	    std::string operationId,
	    const LauncherOperationRequest& request,
	    IProcessRunner& processRunner,
	    TaskExecutionContext& context,
	    const ProcessOutputCallback& outputCallback)
	{
		TaskContextProcessRunner taskProcessRunner(processRunner, context);
		LauncherOperationPlan plan = PlanLauncherOperation(category, operationId, request);
		return std::visit(
		    [&taskProcessRunner, &outputCallback](auto&& typedPlan) -> OperationRecord
		    {
			    using Plan = std::remove_cvref_t<decltype(typedPlan)>;
			    if (!typedPlan.CanRun)
			    {
				    OperationRecord record = std::move(typedPlan.Operation);
				    record.FailureSummary =
				        typedPlan.ReadinessMessages.empty() ? "Operation readiness failed." : typedPlan.ReadinessMessages.back();
				    record.Status = OperationStatus::Skipped;
				    return record;
			    }

			    if constexpr (std::is_same_v<Plan, BuildWorkspaceOperationPlan>)
			    {
				    return RunBuildWorkspaceOperationPlan(std::move(typedPlan), taskProcessRunner, outputCallback);
			    }
			    else if constexpr (std::is_same_v<Plan, CookOperationPlan>)
			    {
				    return RunCookOperationPlan(std::move(typedPlan), taskProcessRunner, outputCallback);
			    }
			    else if constexpr (std::is_same_v<Plan, MaintenanceOperationPlan>)
			    {
				    return RunMaintenanceOperationPlan(std::move(typedPlan), taskProcessRunner, outputCallback);
			    }
			    else
			    {
				    return RunLaunchOperationPlan(std::move(typedPlan), taskProcessRunner, outputCallback);
			    }
		    },
		    std::move(plan));
	}
}

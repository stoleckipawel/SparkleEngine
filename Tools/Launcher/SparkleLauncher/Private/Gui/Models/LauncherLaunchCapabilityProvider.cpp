#include "LauncherCapabilityProviders.h"

#include "LauncherOperationRequestFactory.h"
#include "LauncherOperationRequestMapping.h"

#include "SparkleLauncher/BuildWorkspaceOperations.h"
#include "SparkleLauncher/LaunchOperations.h"

#include <string>

namespace SparkleLauncher
{
	static LaunchOperationPlan PlanRequestedProductLaunch(const LauncherOperationRequest& request)
	{
		return PlanLaunchOperation(request.OperationId.toStdString(), LauncherOperationRequestMapping::Launch(request));
	}

	std::string RegisterLaunchCapabilities(LauncherCapabilityRegistry& registry, const LauncherCapabilityContext& context)
	{
		const LauncherOperationRequest request = context.Request;
		if (!context.IsLaunchGoal())
		{
			return {};
		}

		std::string error = registry.Register(
		    {context.ProjectCapabilityId(),
		        {},
		        [request](bool)
		        {
			        const LaunchOperationPlan plan =
			            PlanLaunchOperation(request.OperationId.toStdString(), LauncherOperationRequestMapping::Launch(request));
			        return plan.Readiness.ContentDirectoryReady
			            ? LauncherCapabilityEvaluation::Ready()
			            : LauncherCapabilityEvaluation::Blocked(BuildCapabilityReadinessSummary(plan.ReadinessMessages));
		        }});
		if (!error.empty())
		{
			return error;
		}

		error = registry.Register(
		    {context.ProductCapabilityId(),
		        {std::string(LauncherCapabilityId::BuildFiles)},
		        [request, buildOperationId = context.ProductBuildOperationId()](bool)
		        {
			        if (PlanRequestedProductLaunch(request).Readiness.ExecutableReady)
			        {
				        return LauncherCapabilityEvaluation::Ready();
			        }

			        const BuildWorkspaceOperationPlan buildPlan = PlanBuildWorkspaceOperation(
			            buildOperationId.toStdString(),
			            LauncherOperationRequestMapping::BuildWorkspace(request));
			        return buildPlan.CanRun
			            ? LauncherCapabilityEvaluation::RunOperation(BuildQuickStartOperationRequest(request, buildOperationId))
			            : LauncherCapabilityEvaluation::DependenciesRequired(BuildCapabilityReadinessSummary(buildPlan.ReadinessMessages));
		        }});
		if (!error.empty())
		{
			return error;
		}

		return registry.Register(
		    {request.OperationId.toStdString(),
		        {context.ProjectCapabilityId(),
		            context.ProductCapabilityId(),
		            std::string(LauncherCapabilityId::SelectedLevels),
		            std::string(LauncherCapabilityId::CookedContent)},
		        [request](bool)
		        {
			        const LaunchOperationPlan plan =
			            PlanLaunchOperation(request.OperationId.toStdString(), LauncherOperationRequestMapping::Launch(request));
			        return plan.CanRun
			            ? LauncherCapabilityEvaluation::RunOperation(BuildQuickStartOperationRequest(request, request.OperationId))
			            : LauncherCapabilityEvaluation::DependenciesRequired(BuildCapabilityReadinessSummary(plan.ReadinessMessages));
		        }});
	}
}

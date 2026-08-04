#include "LauncherCapabilityProviders.h"

#include "LauncherOperationRequestFactory.h"
#include "LauncherOperationRequestMapping.h"

#include "SparkleLauncher/LaunchOperations.h"

#include <string>

namespace SparkleLauncher
{
	std::string RegisterLaunchCapabilities(LauncherCapabilityRegistry& registry, const LauncherCapabilityContext& context)
	{
		const LauncherOperationRequest request = context.Request;
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

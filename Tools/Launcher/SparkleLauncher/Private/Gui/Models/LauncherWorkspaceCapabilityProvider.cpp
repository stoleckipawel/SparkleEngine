#include "LauncherCapabilityProviders.h"

#include "LauncherOperationRequestFactory.h"
#include "LauncherOperationRequestMapping.h"

#include "SparkleLauncher/BuildWorkspaceOperations.h"

#include <string>

namespace SparkleLauncher
{
	std::string RegisterWorkspaceCapabilities(LauncherCapabilityRegistry& registry, const LauncherCapabilityContext& context)
	{
		const LauncherOperationRequest request = context.Request;
		std::string error = registry.Register(
		    {std::string(LauncherCapabilityId::BuildFiles),
		        {std::string(LauncherCapabilityId::SourceDependencies)},
		        [request](bool)
		        {
			        const BuildWorkspaceOperationRequest workspaceRequest = LauncherOperationRequestMapping::BuildWorkspace(request);
			        const BuildWorkspaceOperationPlan generatePlan =
			            PlanBuildWorkspaceOperation("workspace.generate-build-files", workspaceRequest);
			        if (generatePlan.Freshness.Current)
			        {
				        return LauncherCapabilityEvaluation::Ready();
			        }

			        return generatePlan.CanRun ? LauncherCapabilityEvaluation::RunOperation(
			                                         BuildQuickStartOperationRequest(request, "workspace.generate-build-files"))
			                                   : LauncherCapabilityEvaluation::DependenciesRequired(
			                                         BuildCapabilityReadinessSummary(generatePlan.ReadinessMessages));
		        }});
		return error;
	}
}

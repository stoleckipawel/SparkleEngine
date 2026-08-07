#include "LauncherCapabilityProviders.h"

#include "LauncherOperationRequestFactory.h"
#include "LauncherOperationRequestMapping.h"

#include "SparkleLauncher/BuildWorkspaceOperations.h"
#include "SparkleLauncher/LevelRunOperations.h"

#include <string>

namespace SparkleLauncher
{
	std::string RegisterLevelRunCapabilities(LauncherCapabilityRegistry& registry, const LauncherCapabilityContext& context)
	{
		if (!context.IsLevelRunGoal())
		{
			return {};
		}

		const LauncherOperationRequest request = context.Request;
		std::string error = registry.Register(
		    {context.ProjectCapabilityId(),
		        {},
		        [request](bool)
		        {
			        const LevelRunOperationPlan plan =
			            PlanLevelRunOperation(request.OperationId.toStdString(), LauncherOperationRequestMapping::LevelRun(request));
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
			        const LevelRunOperationPlan runPlan =
			            PlanLevelRunOperation(request.OperationId.toStdString(), LauncherOperationRequestMapping::LevelRun(request));
			        if (runPlan.Readiness.ExecutableReady)
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
		            std::string(LauncherCapabilityId::SelectedLevels),
		            context.ProductCapabilityId(),
		            std::string(LauncherCapabilityId::CookedContent)},
		        [request](bool)
		        {
			        const LevelRunOperationPlan plan =
			            PlanLevelRunOperation(request.OperationId.toStdString(), LauncherOperationRequestMapping::LevelRun(request));
			        return plan.CanRun
			            ? LauncherCapabilityEvaluation::RunOperation(BuildQuickStartOperationRequest(request, request.OperationId))
			            : LauncherCapabilityEvaluation::DependenciesRequired(BuildCapabilityReadinessSummary(plan.ReadinessMessages));
		        }});
	}
}

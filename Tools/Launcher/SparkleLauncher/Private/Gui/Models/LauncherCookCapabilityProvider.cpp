#include "LauncherCapabilityProviders.h"

#include "LauncherOperationRequestFactory.h"
#include "LauncherOperationRequestMapping.h"

#include "SparkleLauncher/BuildWorkspaceOperations.h"
#include "SparkleLauncher/CookOperations.h"
#include "SparkleLauncher/LevelRunOperations.h"

#include <string>
#include <utility>

namespace SparkleLauncher
{
	static QString SelectCookOperationId(const LauncherOperationRequest& request, bool invalidated)
	{
		if (invalidated)
		{
			return QStringLiteral("cook.all");
		}

		const LevelRunOperationPlan runPlan =
		    PlanLevelRunOperation(request.OperationId.toStdString(), LauncherOperationRequestMapping::LevelRun(request));
		const bool meshesMissing = !runPlan.Readiness.CookedMeshesReady;
		const bool texturesMissing = !runPlan.Readiness.CookedTexturesReady;
		const bool shadersMissing = !runPlan.Readiness.CookedShadersReady;
		const int missingCount = static_cast<int>(meshesMissing) + static_cast<int>(texturesMissing) + static_cast<int>(shadersMissing);
		if (missingCount > 1)
		{
			return QStringLiteral("cook.all");
		}
		if (meshesMissing)
		{
			return QStringLiteral("cook.assets");
		}
		if (texturesMissing)
		{
			return QStringLiteral("cook.textures");
		}
		return QStringLiteral("cook.shaders");
	}

	std::string RegisterCookCapabilities(LauncherCapabilityRegistry& registry, const LauncherCapabilityContext& context)
	{
		if (!context.IsLevelRunGoal())
		{
			return {};
		}

		const LauncherOperationRequest request = context.Request;
		std::string error = registry.Register(
		    {std::string(LauncherCapabilityId::CookingTools),
		        {std::string(LauncherCapabilityId::BuildFiles)},
		        [request](bool)
		        {
			        LauncherOperationRequest cookRequest = BuildQuickStartOperationRequest(request, "cook.all");
			        const CookOperationPlan cookPlan = PlanCookOperation("cook.all", LauncherOperationRequestMapping::Cook(cookRequest));
			        if (cookPlan.CanRun && !cookPlan.Steps.empty())
			        {
				        return LauncherCapabilityEvaluation::Ready();
			        }

			        LauncherOperationRequest buildRequest = BuildQuickStartOperationRequest(request, "cook.tools.prepare");
			        buildRequest.EditorProfile = QString::fromStdString(cookPlan.ToolProfile);
			        const BuildWorkspaceOperationPlan buildPlan =
			            PlanBuildWorkspaceOperation("cook.tools.prepare", LauncherOperationRequestMapping::BuildWorkspace(buildRequest));
			        return buildPlan.CanRun
			            ? LauncherCapabilityEvaluation::RunOperation(std::move(buildRequest))
			            : LauncherCapabilityEvaluation::DependenciesRequired(BuildCapabilityReadinessSummary(buildPlan.ReadinessMessages));
		        }});
		if (!error.empty())
		{
			return error;
		}

		return registry.Register(
		    {std::string(LauncherCapabilityId::CookedContent),
		        {std::string(LauncherCapabilityId::SelectedLevels), std::string(LauncherCapabilityId::CookingTools)},
		        [request](bool invalidated)
		        {
			        const LevelRunOperationPlan runPlan =
			            PlanLevelRunOperation(request.OperationId.toStdString(), LauncherOperationRequestMapping::LevelRun(request));
			        const bool cookedContentReady = !invalidated && runPlan.Readiness.CookedMeshesReady
			            && runPlan.Readiness.CookedTexturesReady && runPlan.Readiness.CookedShadersReady;
			        if (cookedContentReady)
			        {
				        return LauncherCapabilityEvaluation::Ready();
			        }

			        const QString cookOperationId = SelectCookOperationId(request, invalidated);
			        LauncherOperationRequest cookRequest = BuildQuickStartOperationRequest(request, cookOperationId);
			        const CookOperationPlan cookPlan =
			            PlanCookOperation(cookOperationId.toStdString(), LauncherOperationRequestMapping::Cook(cookRequest));
			        return cookPlan.CanRun && !cookPlan.Steps.empty()
			            ? LauncherCapabilityEvaluation::RunOperation(std::move(cookRequest))
			            : LauncherCapabilityEvaluation::DependenciesRequired(BuildCapabilityReadinessSummary(cookPlan.ReadinessMessages));
		        }});
	}
}

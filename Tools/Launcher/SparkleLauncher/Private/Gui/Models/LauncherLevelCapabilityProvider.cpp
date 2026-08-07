#include "LauncherCapabilityProviders.h"

#include "LauncherLevelUiModel.h"
#include "LauncherOperationRequestFactory.h"
#include "LauncherOperationRequestMapping.h"

#include "SparkleLauncher/LevelOperations.h"

#include <algorithm>
#include <string>
#include <utility>

namespace SparkleLauncher
{
	std::string RegisterLevelCapabilities(LauncherCapabilityRegistry& registry, const LauncherCapabilityContext& context)
	{
		if (!context.IsLevelRunGoal())
		{
			return {};
		}

		const LauncherOperationRequest request = context.Request;
		const LauncherLevelUiModel& levelModel = context.LevelModel;
		return registry.Register(
		    {std::string(LauncherCapabilityId::SelectedLevels),
		        {},
		        [request, &levelModel](bool)
		        {
			        if (!levelModel.Loaded)
			        {
				        return LauncherCapabilityEvaluation::Blocked(
				            levelModel.LoadError.isEmpty() ? std::string("The level catalog could not be loaded.")
				                                           : levelModel.LoadError.toStdString());
			        }

			        const QString levelId = request.RequestedLevelIds.section(',', 0, 0).trimmed();
			        const auto found = std::find_if(
			            levelModel.Levels.begin(),
			            levelModel.Levels.end(),
			            [&levelId](const LauncherLevelUiEntry& level) { return level.Id == levelId; });
			        if (levelId.isEmpty() || found == levelModel.Levels.end())
			        {
				        return LauncherCapabilityEvaluation::Blocked("The requested catalog level does not exist.");
			        }
			        if (!found->RuntimeSupported || !found->CanSelect)
			        {
				        const QString reason = found->UnsupportedReason.isEmpty() ? found->Status : found->UnsupportedReason;
				        return LauncherCapabilityEvaluation::Blocked(
				            QStringLiteral("%1: %2").arg(found->DisplayName, reason).toStdString());
			        }
			        if (found->Selected && found->Ready)
			        {
				        return LauncherCapabilityEvaluation::Ready();
			        }
			        if (!found->CanSync)
			        {
				        return LauncherCapabilityEvaluation::Blocked(
				            QStringLiteral("%1 is not ready and has no available acquisition path.").arg(found->DisplayName).toStdString());
			        }

			        LauncherOperationRequest syncRequest = BuildQuickStartOperationRequest(request, "levels.sync", {levelId});
			        const LevelOperationPlan plan = PlanLevelOperation("levels.sync", LauncherOperationRequestMapping::Levels(syncRequest));
			        return plan.CanRun ? LauncherCapabilityEvaluation::RunOperation(std::move(syncRequest))
			                           : LauncherCapabilityEvaluation::Blocked(BuildCapabilityReadinessSummary(plan.ReadinessMessages));
		        }});
	}
}

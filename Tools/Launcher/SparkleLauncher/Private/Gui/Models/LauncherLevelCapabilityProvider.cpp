#include "LauncherCapabilityProviders.h"

#include "LauncherLevelUiModel.h"
#include "LauncherOperationRequestFactory.h"
#include "LauncherOperationRequestMapping.h"

#include "SparkleLauncher/BuildWorkspaceOperations.h"

#include <QtCore/QStringList>

#include <string>
#include <utility>

namespace SparkleLauncher
{
	std::string RegisterLevelCapabilities(LauncherCapabilityRegistry& registry, const LauncherCapabilityContext& context)
	{
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

			        QStringList missingLevelIds;
			        QStringList blockers;
			        for (const LauncherLevelUiEntry& level : levelModel.Levels)
			        {
				        if (!level.Selected || level.Ready)
				        {
					        continue;
				        }
				        if (level.CanSync)
				        {
					        missingLevelIds.push_back(level.Id);
					        continue;
				        }

				        const QString reason = level.UnsupportedReason.isEmpty() ? level.Status : level.UnsupportedReason;
				        blockers.push_back(QStringLiteral("%1: %2").arg(level.DisplayName, reason));
			        }
			        if (!blockers.isEmpty())
			        {
				        return LauncherCapabilityEvaluation::Blocked(blockers.join('\n').toStdString());
			        }
			        if (missingLevelIds.isEmpty())
			        {
				        return LauncherCapabilityEvaluation::Ready();
			        }

			        LauncherOperationRequest syncRequest =
			            BuildQuickStartOperationRequest(request, "workspace.sync-levels", missingLevelIds);
			        const BuildWorkspaceOperationPlan plan =
			            PlanBuildWorkspaceOperation("workspace.sync-levels", LauncherOperationRequestMapping::BuildWorkspace(syncRequest));
			        return plan.CanRun ? LauncherCapabilityEvaluation::RunOperation(std::move(syncRequest))
			                           : LauncherCapabilityEvaluation::Blocked(BuildCapabilityReadinessSummary(plan.ReadinessMessages));
		        }});
	}
}

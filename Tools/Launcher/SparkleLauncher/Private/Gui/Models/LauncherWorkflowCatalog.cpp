#include "LauncherWorkflowCatalog.h"

#include "LauncherUiModel.h"

namespace SparkleLauncher
{
	QString LauncherHomeOperationId()
	{
		return "home.quick-start";
	}

	QVector<LauncherWorkflowDefinition> CreateLauncherWorkflowCatalog()
	{
		return {
		    {LauncherWorkflowPageKind::Home, {LauncherHomeOperationId()}},
		    {LauncherWorkflowPageKind::Sync, {"workspace.sync-code"}},
		    {LauncherWorkflowPageKind::Build, {"workspace.build"}},
		    {LauncherWorkflowPageKind::Cook, {"cook.shaders", "cook.textures", "cook.assets", "cook.all"}},
		    {LauncherWorkflowPageKind::Clean, {"workspace.clean"}},
		};
	}

	QString LauncherOperationDisplayNameOverride(const QString& operationId)
	{
		return LauncherUiModelForOperation(operationId).DisplayName;
	}

	QString LauncherOperationImpactText(const QString& operationId)
	{
		return LauncherUiModelForOperation(operationId).ImpactText;
	}

	QString LauncherWorkflowPrimaryVerb(const QString& operationId)
	{
		const QString primaryVerb = LauncherUiModelForOperation(operationId).PrimaryVerb;
		return primaryVerb.isEmpty() ? QString("Run") : primaryVerb;
	}
}

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
		    {"Quick Start", "Launch first", {LauncherHomeOperationId()}, "home"},
		    {"Sync", "Code and levels", {"workspace.sync-source-tiers", "workspace.sync-levels"}, "sync"},
		    {"Build",
		        "Generate and build",
		        {"workspace.generate-build-files",
		            "launcher.build.self",
		            "workspace.build.editor",
		            "workspace.build.runtime",
		            "cook.tools.prepare",
		            "workspace.build-all"},
		        "build"},
		    {"Cook", "Asset refresh", {"cook.shaders", "cook.textures", "cook.assets", "cook.all"}, "cook"},
		    {"Launch", "Open what is ready", {"launch.run", "workspace.open-ide"}, "launch"},
		    {"Clean", "Clean generated state", {"workspace.clean"}, "clean"},
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

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
		    {"Build", "Prepare and build", {"workspace.sync-source-tiers", "workspace.generate-build-files", "launcher.build.self", "project.build.editor", "project.build.runtime", "cook.tools.prepare", "workspace.build-all"}, "build"},
		    {"Cook", "Optional content refresh", {"cook.shaders", "cook.textures", "cook.assets", "cook.project"}, "cook"},
		    {"Launch", "Open what is ready", {"project.run", "workspace.open-ide"}, "launch"},
		    {"Package", "Release assembly", {"package.release"}, "package"},
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

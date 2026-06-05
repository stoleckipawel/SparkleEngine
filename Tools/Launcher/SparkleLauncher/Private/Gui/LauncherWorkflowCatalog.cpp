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
		    {"Launch", "Open what is ready", {"project.run", "workspace.open-solution"}, "launch"},
		    {"Sync", "Bring capabilities local", {"toolchain.check", "workspace.setup"}, "sync"},
		    {"Build", "Optional local rebuilds", {"workspace.generate-solution", "workspace.build-all", "launcher.build.self", "project.build.editor", "project.build.runtime", "cook.tools.prepare"}, "build"},
		    {"Cook", "Optional content refresh", {"cook.project", "cook.shaders", "cook.textures", "cook.assets"}, "cook"},
		    {"Test", "Quality gates", {"project.run.smoke", "quality.format"}, "test"},
		    {"Package", "Release assembly", {"package.release"}, "package"},
		    {"Maintain", "Clean generated state", {"workspace.clean"}, "maintain"},
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

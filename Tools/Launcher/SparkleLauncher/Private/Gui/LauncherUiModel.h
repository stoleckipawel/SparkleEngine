#pragma once

#include <QtCore/QString>

namespace SparkleLauncher
{
	enum class LauncherWorkflowPageKind
	{
		Home,
		Launch,
		Sync,
		Build,
		Cook,
		Test,
		Package,
		Maintain,
		Unknown
	};

	enum class LauncherActionImpactKind
	{
		None,
		Diagnostics,
		SourceDependencies,
		WorkspaceFiles,
		BuildOutputs,
		CookedOutputs,
		LaunchProcess,
		TestRun,
		PackageOutputs,
		GeneratedState
	};

	struct LauncherOperationUiModel
	{
		QString OperationId;
		QString DisplayName;
		QString PrimaryVerb;
		QString ImpactText;
		QString VisualTitle;
		QString VisualText;
		QString VisualAssetName;
		LauncherWorkflowPageKind PageKind = LauncherWorkflowPageKind::Unknown;
		LauncherActionImpactKind ImpactKind = LauncherActionImpactKind::None;
	};

	LauncherOperationUiModel LauncherUiModelForOperation(const QString& operationId);
	QString LauncherWorkflowPageKindName(LauncherWorkflowPageKind pageKind);
	QString LauncherActionImpactKindName(LauncherActionImpactKind impactKind);
}

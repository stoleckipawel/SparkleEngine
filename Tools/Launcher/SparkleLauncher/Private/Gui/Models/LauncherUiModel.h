#pragma once

#include <QtCore/QString>

#include <cstdint>

namespace SparkleLauncher
{
	enum class LauncherWorkflowPageKind : std::uint8_t
	{
		Home,
		Sync,
		Build,
		Cook,
		Clean,
		Unknown
	};

	enum class LauncherActionImpactKind : std::uint8_t
	{
		None,
		SourceDependencies,
		LevelAssets,
		WorkspaceFiles,
		BuildOutputs,
		CookedOutputs,
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

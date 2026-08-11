#pragma once

#include "LauncherBackend.h"

#include "SparkleLauncher/BuildWorkspaceOperations.h"

#include <QtCore/QString>
#include <QtCore/QStringList>

#include <filesystem>

namespace SparkleLauncher
{
	struct ActionCleanTargetContext;
	class LauncherContentModel;
	class LauncherSettings;

	WorkspaceIde ResolveSelectedWorkspaceIde(const LauncherSettings& settings);
	QString ResolveSelectedWorkspaceIdeName(const LauncherSettings& settings);
	WorkspaceCompiler ResolveSelectedWorkspaceCompiler(const LauncherSettings& settings);

	BuildWorkspaceOperationRequest BuildWorkspacePlanRequest(
	    const std::filesystem::path& repositoryRoot,
	    const LauncherContentModel& contentModel,
	    const LauncherSettings& settings);

	LauncherOperationRequest BuildLauncherOperationRequest(
	    const std::filesystem::path& repositoryRoot,
	    const LauncherContentModel& contentModel,
	    const LauncherSettings& settings,
	    const QString& operationId);
	LauncherOperationRequest BuildQuickStartOperationRequest(
	    const LauncherOperationRequest& goalRequest,
	    const QString& operationId,
	    const QStringList& requestedLevelIds = {});

	ActionCleanTargetContext BuildActionCleanTargetContext(
	    const std::filesystem::path& repositoryRoot,
	    const LauncherContentModel& contentModel,
	    const LauncherSettings& settings,
	    const std::filesystem::path& runningLauncherPath,
	    const QString& operationId);

	LauncherOperationRequest BuildActionCleanOperationRequest(
	    const std::filesystem::path& repositoryRoot,
	    const LauncherContentModel& contentModel,
	    const LauncherSettings& settings,
	    const std::filesystem::path& runningLauncherPath,
	    const QString& operationId);

	LauncherOperationRequest BuildScopedCleanOperationRequest(
	    const std::filesystem::path& repositoryRoot,
	    const LauncherContentModel& contentModel,
	    const LauncherSettings& settings,
	    const QString& cleanScope,
	    const std::filesystem::path& runningLauncherPath = {});

}

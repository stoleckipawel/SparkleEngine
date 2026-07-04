#pragma once

#include "LauncherBackend.h"

#include "SparkleLauncher/BuildWorkspaceOperations.h"

#include <QtCore/QString>

#include <filesystem>

namespace SparkleLauncher
{
	struct ActionCleanTargetContext;
	class LauncherProjectModel;
	class LauncherSettings;

	WorkspaceIde ResolveSelectedWorkspaceIde(const LauncherSettings& settings);
	QString ResolveSelectedWorkspaceIdeName(const LauncherSettings& settings);
	QString ResolveShaderTargetSelection(const LauncherSettings& settings);
	QString ResolveShaderDebugArtifactDirectory(
	    const std::filesystem::path& repositoryRoot,
	    const LauncherProjectModel& projectModel,
	    const LauncherSettings& settings);

	BuildWorkspaceOperationRequest BuildWorkspacePlanRequest(
	    const std::filesystem::path& repositoryRoot,
	    const LauncherProjectModel& projectModel,
	    const LauncherSettings& settings);

	LauncherOperationRequest BuildLauncherOperationRequest(
	    const std::filesystem::path& repositoryRoot,
	    const LauncherProjectModel& projectModel,
	    const LauncherSettings& settings,
	    const QString& operationId);

	ActionCleanTargetContext BuildActionCleanTargetContext(
	    const std::filesystem::path& repositoryRoot,
	    const LauncherProjectModel& projectModel,
	    const LauncherSettings& settings,
	    const std::filesystem::path& runningLauncherPath,
	    const QString& operationId);

	LauncherOperationRequest BuildActionCleanOperationRequest(
	    const std::filesystem::path& repositoryRoot,
	    const LauncherProjectModel& projectModel,
	    const LauncherSettings& settings,
	    const std::filesystem::path& runningLauncherPath,
	    const QString& operationId);

	LauncherOperationRequest BuildScopedCleanOperationRequest(
	    const std::filesystem::path& repositoryRoot,
	    const LauncherProjectModel& projectModel,
	    const LauncherSettings& settings,
	    const QString& cleanScope,
	    const std::filesystem::path& runningLauncherPath = {});

}

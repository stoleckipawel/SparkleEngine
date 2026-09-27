#include "LauncherStatePaths.h"

#include "Core/Public/Paths/WorkspaceUserStatePaths.h"

namespace SparkleLauncher
{
	LauncherStatePaths ResolveLauncherStatePaths(const std::filesystem::path& repositoryRoot)
	{
		const Filesystem::WorkspaceUserStatePaths workspaceState = Filesystem::ResolveWorkspaceUserStatePaths(repositoryRoot);
		const std::filesystem::path root = workspaceState.LauncherRoot;
		return LauncherStatePaths{
		    .Root = root,
		    .LogsRoot = root / "Logs",
		    .ContentArchivesRoot = root / "ContentArchives",
		    .LiveInstancesRoot = root / "Live",
		    .ActivityFile = root / "Activity.json",
		    .SettingsFile = root / "Settings.json"};
	}

	std::filesystem::path ResolveLauncherOperationLogPath(
	    const std::filesystem::path& repositoryRoot,
	    std::string_view operationId,
	    std::string_view logFileName)
	{
		const LauncherStatePaths statePaths = ResolveLauncherStatePaths(repositoryRoot);
		return statePaths.LogsRoot / std::string(operationId) / std::string(logFileName);
	}
}

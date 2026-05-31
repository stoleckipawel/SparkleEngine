#include "SparkleLauncher/LauncherPaths.h"

namespace SparkleLauncher
{
	LauncherStatePaths GetLauncherStatePaths(const std::filesystem::path& repositoryRoot)
	{
		LauncherStatePaths paths;
		paths.RootDirectory = GetLauncherStateDirectory(repositoryRoot);
		paths.LogsDirectory = paths.RootDirectory / "Logs";
		paths.ActivityPath = paths.RootDirectory / "Activity.json";
		paths.ActionHistoryPath = paths.RootDirectory / "ActionHistory.tsv";
		paths.SettingsPath = paths.RootDirectory / "Settings.json";
		return paths;
	}

	std::filesystem::path GetLauncherStateDirectory(const std::filesystem::path& repositoryRoot)
	{
		return GetBuildDirectory(repositoryRoot) / "Launcher";
	}

	std::filesystem::path GetLauncherOperationLogPath(
	    const std::filesystem::path& repositoryRoot,
	    std::string_view operationId,
	    std::string_view logFileName)
	{
		return GetLauncherStatePaths(repositoryRoot).LogsDirectory / std::string(operationId) / std::string(logFileName);
	}

	std::filesystem::path GetBuildDirectory(const std::filesystem::path& repositoryRoot)
	{
		return repositoryRoot / "build";
	}

	std::filesystem::path GetBuildBinaryDirectory(const std::filesystem::path& repositoryRoot, std::string_view profileName)
	{
		return GetBuildDirectory(repositoryRoot) / "bin" / std::string(profileName);
	}

	std::filesystem::path GetCookedProjectDirectory(const std::filesystem::path& repositoryRoot, std::string_view projectName)
	{
		return GetBuildDirectory(repositoryRoot) / "Cooked" / std::string(projectName);
	}
}

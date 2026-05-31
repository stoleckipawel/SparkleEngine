#pragma once

#include <filesystem>
#include <string_view>

namespace SparkleLauncher
{
	struct LauncherStatePaths
	{
		std::filesystem::path RootDirectory;
		std::filesystem::path LogsDirectory;
		std::filesystem::path ActivityPath;
		std::filesystem::path ActionHistoryPath;
		std::filesystem::path SettingsPath;
	};

	LauncherStatePaths GetLauncherStatePaths(const std::filesystem::path& repositoryRoot);
	std::filesystem::path GetLauncherStateDirectory(const std::filesystem::path& repositoryRoot);
	std::filesystem::path GetLauncherOperationLogPath(
	    const std::filesystem::path& repositoryRoot,
	    std::string_view operationId,
	    std::string_view logFileName);
	std::filesystem::path GetBuildDirectory(const std::filesystem::path& repositoryRoot);
	std::filesystem::path GetBuildBinaryDirectory(const std::filesystem::path& repositoryRoot, std::string_view profileName);
	std::filesystem::path GetCookedProjectDirectory(const std::filesystem::path& repositoryRoot, std::string_view projectName);
}

#pragma once

#include <filesystem>
#include <string_view>

namespace SparkleLauncher
{
	struct LauncherStatePaths final
	{
		std::filesystem::path Root;
		std::filesystem::path LogsRoot;
		std::filesystem::path ContentArchivesRoot;
		std::filesystem::path LiveInstancesRoot;
		std::filesystem::path ActivityFile;
		std::filesystem::path SettingsFile;
	};

	LauncherStatePaths ResolveLauncherStatePaths(const std::filesystem::path& repositoryRoot);
	std::filesystem::path ResolveLauncherOperationLogPath(const std::filesystem::path& repositoryRoot, std::string_view operationId, std::string_view logFileName);
}

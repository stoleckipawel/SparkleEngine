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
		std::filesystem::path SettingsPath;
	};

	LauncherStatePaths GetLauncherStatePaths(const std::filesystem::path& repositoryRoot);
	std::filesystem::path GetLauncherStateDirectory(const std::filesystem::path& repositoryRoot);
	std::filesystem::path GetLauncherOperationLogPath(
	    const std::filesystem::path& repositoryRoot,
	    std::string_view operationId,
	    std::string_view logFileName);
	std::filesystem::path GetBuildDirectory(const std::filesystem::path& repositoryRoot);
	std::filesystem::path GetArtifactDirectory(const std::filesystem::path& repositoryRoot);
	std::filesystem::path GetDeveloperArtifactDirectory(const std::filesystem::path& repositoryRoot);
	std::filesystem::path GetDeveloperLibraryDirectory(
	    const std::filesystem::path& repositoryRoot,
	    std::string_view owner,
	    std::string_view profileName);
	std::filesystem::path GetLauncherArtifactDirectory(const std::filesystem::path& repositoryRoot, std::string_view profileName);
	std::filesystem::path GetDevelopmentToolArtifactDirectory(
	    const std::filesystem::path& repositoryRoot,
	    std::string_view toolName,
	    std::string_view profileName);
	std::filesystem::path GetProjectArtifactDirectory(const std::filesystem::path& repositoryRoot, std::string_view projectName);
	std::filesystem::path GetProjectTargetArtifactDirectory(
	    const std::filesystem::path& repositoryRoot,
	    std::string_view projectName,
	    std::string_view productRole,
	    std::string_view profileName);
	std::filesystem::path GetCookedProjectsArtifactDirectory(const std::filesystem::path& repositoryRoot);
	std::filesystem::path GetDiagnosticsDirectory(const std::filesystem::path& repositoryRoot);
	std::filesystem::path GetSymbolDirectory(const std::filesystem::path& repositoryRoot);
	std::filesystem::path GetCookedProjectDirectory(const std::filesystem::path& repositoryRoot, std::string_view projectName);
	std::filesystem::path GetSharedCookedProjectDirectory(const std::filesystem::path& repositoryRoot);
}

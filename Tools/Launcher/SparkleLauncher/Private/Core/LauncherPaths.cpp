#include "SparkleLauncher/LauncherPaths.h"

#include "Core/Public/Environment/EnvironmentVariables.h"

namespace SparkleLauncher
{
	static std::filesystem::path ResolveConfiguredBuildDirectory(const std::filesystem::path& repositoryRoot)
	{
		std::string configuredPath;
		if (Environment::TryGetVariable("SPARKLE_BUILD_DIR", configuredPath) ||
		    Environment::TryGetVariable("SPARKLE_BUILD_DIRECTORY", configuredPath))
		{
			const std::filesystem::path candidate(configuredPath);
			return candidate.is_absolute() ? candidate : (repositoryRoot / candidate);
		}

		return repositoryRoot / "build" / "windows-msvc-x64-dev";
	}

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
		return GetDeveloperArtifactDirectory(repositoryRoot) / "launcher-state";
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
		return ResolveConfiguredBuildDirectory(repositoryRoot);
	}

	std::filesystem::path GetArtifactDirectory(const std::filesystem::path& repositoryRoot)
	{
		return repositoryRoot / "artifacts";
	}

	std::filesystem::path GetDeveloperArtifactDirectory(const std::filesystem::path& repositoryRoot)
	{
		return GetArtifactDirectory(repositoryRoot) / "dev";
	}

	std::filesystem::path GetDeveloperLibraryDirectory(const std::filesystem::path& repositoryRoot, std::string_view owner, std::string_view profileName)
	{
		return GetDeveloperArtifactDirectory(repositoryRoot) / "libraries" / std::string(owner) / std::string(profileName);
	}

	std::filesystem::path GetLauncherArtifactDirectory(const std::filesystem::path& repositoryRoot, std::string_view profileName)
	{
		return GetDeveloperArtifactDirectory(repositoryRoot) / "launcher" / std::string(profileName);
	}

	std::filesystem::path GetDevelopmentToolArtifactDirectory(
	    const std::filesystem::path& repositoryRoot,
	    std::string_view toolName,
	    std::string_view profileName)
	{
		return GetDeveloperArtifactDirectory(repositoryRoot) / "tools" / std::string(toolName) / std::string(profileName);
	}

	std::filesystem::path GetProjectArtifactDirectory(const std::filesystem::path& repositoryRoot, std::string_view projectName)
	{
		return GetDeveloperArtifactDirectory(repositoryRoot) / "projects" / std::string(projectName);
	}

	std::filesystem::path GetProjectTargetArtifactDirectory(
	    const std::filesystem::path& repositoryRoot,
	    std::string_view projectName,
	    std::string_view productRole,
	    std::string_view profileName)
	{
		return GetProjectArtifactDirectory(repositoryRoot, projectName) / std::string(productRole) / std::string(profileName);
	}

	std::filesystem::path GetCookedProjectsArtifactDirectory(const std::filesystem::path& repositoryRoot)
	{
		return GetDeveloperArtifactDirectory(repositoryRoot) / "projects";
	}

	std::filesystem::path GetDiagnosticsDirectory(const std::filesystem::path& repositoryRoot)
	{
		return GetArtifactDirectory(repositoryRoot) / "diagnostics";
	}

	std::filesystem::path GetSymbolDirectory(const std::filesystem::path& repositoryRoot)
	{
		return GetArtifactDirectory(repositoryRoot) / "symbols";
	}

	std::filesystem::path GetCookedProjectDirectory(const std::filesystem::path& repositoryRoot, std::string_view projectName)
	{
		return GetProjectArtifactDirectory(repositoryRoot, projectName) / "cooked";
	}

	std::filesystem::path GetSharedCookedProjectDirectory(const std::filesystem::path& repositoryRoot)
	{
		return GetCookedProjectDirectory(repositoryRoot, "Shared");
	}
}

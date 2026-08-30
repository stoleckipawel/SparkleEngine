#include "SparkleLauncher/LauncherPaths.h"

#include "Core/Public/Environment/EnvironmentVariables.h"

#include <sstream>

namespace SparkleLauncher
{
	static std::filesystem::path ResolveLauncherUserDataRoot()
	{
		std::string configuredPath;
		if (Environment::TryGetVariable("SPARKLE_LAUNCHER_STATE_DIR", configuredPath)
		    || Environment::TryGetVariable("SPARKLE_LAUNCHER_STATE_DIRECTORY", configuredPath))
		{
			return std::filesystem::path(configuredPath);
		}

#if defined(_WIN32)
		std::string localAppData;
		if (Environment::TryGetVariable("LOCALAPPDATA", localAppData) && !localAppData.empty())
		{
			return std::filesystem::path(localAppData) / "SparkleEngine" / "LauncherState";
		}
#endif

		std::string homeDirectory;
		if (Environment::TryGetVariable("HOME", homeDirectory) && !homeDirectory.empty())
		{
			return std::filesystem::path(homeDirectory) / ".sparkle" / "launcher-state";
		}

		return std::filesystem::temp_directory_path() / "SparkleEngine" / "LauncherState";
	}

	static std::string MakeRepositoryStateKey(const std::filesystem::path& repositoryRoot)
	{
		std::error_code errorCode;
		const std::filesystem::path normalizedRoot = std::filesystem::weakly_canonical(repositoryRoot, errorCode);
		const std::string normalizedText = (errorCode ? repositoryRoot : normalizedRoot).generic_string();
		const std::size_t hashValue = std::hash<std::string>{}(normalizedText);

		std::ostringstream key;
		key << repositoryRoot.filename().string() << '-';
		key << std::hex << hashValue;
		return key.str();
	}

	static std::filesystem::path ResolveConfiguredBuildDirectory(const std::filesystem::path& repositoryRoot)
	{
		std::string configuredPath;
		if (Environment::TryGetVariable("SPARKLE_BUILD_DIR", configuredPath)
		    || Environment::TryGetVariable("SPARKLE_BUILD_DIRECTORY", configuredPath))
		{
			const std::filesystem::path candidate(configuredPath);
			return candidate.is_absolute() ? candidate : (repositoryRoot / candidate);
		}

		return repositoryRoot / "build";
	}

	LauncherStatePaths GetLauncherStatePaths(const std::filesystem::path& repositoryRoot)
	{
		LauncherStatePaths paths;
		paths.RootDirectory = GetLauncherStateDirectory(repositoryRoot);
		paths.LogsDirectory = paths.RootDirectory / "Logs";
		paths.ActivityPath = paths.RootDirectory / "Activity.json";
		paths.SettingsPath = paths.RootDirectory / "Settings.json";
		return paths;
	}

	std::filesystem::path GetLauncherStateDirectory(const std::filesystem::path& repositoryRoot)
	{
		return ResolveLauncherUserDataRoot() / MakeRepositoryStateKey(repositoryRoot);
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

	std::filesystem::path GetDeveloperLibraryDirectory(
	    const std::filesystem::path& repositoryRoot,
	    std::string_view owner,
	    std::string_view profileName)
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

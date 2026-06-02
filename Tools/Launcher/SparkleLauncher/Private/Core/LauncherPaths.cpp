#include "SparkleLauncher/LauncherPaths.h"

#include "Core/Public/Environment/EnvironmentVariables.h"

#include <array>
#include <system_error>

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

		const std::array<std::string_view, 4> preferredBuildDirectories = {
		    "build-ninja-msvc-qt",
		    "build",
		    "build-ninja-msvc",
		    "build-msvc",
		};

		std::error_code errorCode;
		for (const std::string_view directoryName : preferredBuildDirectories)
		{
			const std::filesystem::path candidate = repositoryRoot / std::string(directoryName);
			if (!std::filesystem::is_directory(candidate, errorCode))
			{
				errorCode.clear();
				continue;
			}

			if (std::filesystem::exists(candidate / "CMakeCache.txt", errorCode) ||
			    std::filesystem::exists(candidate / "Launcher" / "Settings.json", errorCode) ||
			    std::filesystem::exists(candidate / "bin" / "SparkleLauncher.exe", errorCode))
			{
				return candidate;
			}

			errorCode.clear();
		}

		return repositoryRoot / "build";
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
		return ResolveConfiguredBuildDirectory(repositoryRoot);
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

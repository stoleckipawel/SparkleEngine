#include "NativeBuildOutputReset.h"

#include "SparkleLauncher/LauncherPaths.h"

#include <array>
#include <string_view>
#include <system_error>

namespace SparkleLauncher
{
	static bool ValidateBuildDirectoryResetScope(
	    const std::filesystem::path& repositoryRoot,
	    const std::filesystem::path& buildDirectory,
	    std::string& errorMessage)
	{
		if (repositoryRoot.empty() || buildDirectory.empty())
		{
			errorMessage = "Cannot reset native build outputs without repository and build directory paths.";
			return false;
		}

		std::error_code errorCode;
		const std::filesystem::path normalizedRepositoryRoot = std::filesystem::weakly_canonical(repositoryRoot, errorCode);
		if (errorCode)
		{
			errorMessage = "Failed to resolve the repository root before resetting native build outputs: " + errorCode.message();
			return false;
		}

		errorCode.clear();
		const std::filesystem::path normalizedBuildDirectory = std::filesystem::weakly_canonical(buildDirectory, errorCode);
		if (errorCode)
		{
			errorMessage = "Failed to resolve the build directory before resetting native build outputs: " + errorCode.message();
			return false;
		}

		errorCode.clear();
		const std::filesystem::path repositoryRelativeToBuildDirectory =
		    std::filesystem::relative(normalizedRepositoryRoot, normalizedBuildDirectory, errorCode);
		const bool buildDirectoryContainsRepository =
		    !errorCode && !repositoryRelativeToBuildDirectory.empty() && *repositoryRelativeToBuildDirectory.begin() != "..";
		if (normalizedBuildDirectory.empty() || normalizedBuildDirectory == normalizedBuildDirectory.root_path()
		    || buildDirectoryContainsRepository)
		{
			errorMessage = "Refusing to reset native build outputs because the configured build directory contains the repository: "
			    + normalizedBuildDirectory.string();
			return false;
		}
		return true;
	}

	static bool RemoveGeneratedPath(const std::filesystem::path& path, std::string& errorMessage)
	{
		std::error_code errorCode;
		if (!std::filesystem::exists(path, errorCode))
		{
			if (!errorCode)
			{
				return true;
			}

			errorMessage = "Failed to inspect incompatible build output: " + path.string() + ": " + errorCode.message();
			return false;
		}

		std::filesystem::remove_all(path, errorCode);
		if (!errorCode)
		{
			return true;
		}

		errorMessage = "Failed to remove incompatible build output: " + path.string() + ": " + errorCode.message();
		return false;
	}

	static bool RemoveDependencyBuildState(const std::filesystem::path& dependencyRoot, std::string& errorMessage)
	{
		std::error_code errorCode;
		if (!std::filesystem::is_directory(dependencyRoot, errorCode))
		{
			if (!errorCode)
			{
				return true;
			}

			errorMessage = "Failed to inspect source dependency cache: " + dependencyRoot.string() + ": " + errorCode.message();
			return false;
		}

		std::filesystem::directory_iterator iterator(dependencyRoot, errorCode);
		const std::filesystem::directory_iterator end;
		while (!errorCode && iterator != end)
		{
			const std::filesystem::path path = iterator->path();
			const std::string name = path.filename().string();
			iterator.increment(errorCode);
			if (name.ends_with("-build") || name.ends_with("-subbuild"))
			{
				if (!RemoveGeneratedPath(path, errorMessage))
				{
					return false;
				}
			}
		}
		if (errorCode)
		{
			errorMessage = "Failed to enumerate source dependency cache: " + dependencyRoot.string() + ": " + errorCode.message();
			return false;
		}

		return true;
	}

	static bool RemoveGeneratedBuildTree(const std::filesystem::path& buildDirectory, std::string& errorMessage)
	{
		std::error_code errorCode;
		if (!std::filesystem::is_directory(buildDirectory, errorCode))
		{
			if (!errorCode)
			{
				return true;
			}

			errorMessage = "Failed to inspect generated build tree: " + buildDirectory.string() + ": " + errorCode.message();
			return false;
		}

		std::filesystem::directory_iterator iterator(buildDirectory, errorCode);
		const std::filesystem::directory_iterator end;
		while (!errorCode && iterator != end)
		{
			const std::filesystem::path path = iterator->path();
			iterator.increment(errorCode);
			if (path.filename() == "_deps")
			{
				if (!RemoveDependencyBuildState(path, errorMessage))
				{
					return false;
				}
				continue;
			}

			if (!RemoveGeneratedPath(path, errorMessage))
			{
				return false;
			}
		}
		if (errorCode)
		{
			errorMessage = "Failed to enumerate generated build tree: " + buildDirectory.string() + ": " + errorCode.message();
			return false;
		}

		return true;
	}

	static bool RemoveProjectBuildOutputs(const std::filesystem::path& projectsDirectory, std::string& errorMessage)
	{
		std::error_code errorCode;
		if (!std::filesystem::is_directory(projectsDirectory, errorCode))
		{
			if (!errorCode)
			{
				return true;
			}

			errorMessage = "Failed to inspect project artifacts: " + projectsDirectory.string() + ": " + errorCode.message();
			return false;
		}

		std::filesystem::recursive_directory_iterator iterator(
		    projectsDirectory,
		    std::filesystem::directory_options::skip_permission_denied,
		    errorCode);
		const std::filesystem::recursive_directory_iterator end;
		while (!errorCode && iterator != end)
		{
			const std::filesystem::path path = iterator->path();
			if (!iterator->is_directory(errorCode))
			{
				iterator.increment(errorCode);
				continue;
			}

			const std::string name = path.filename().string();
			if (name == "cooked")
			{
				iterator.disable_recursion_pending();
				iterator.increment(errorCode);
				continue;
			}
			if (name == "editor" || name == "runtime")
			{
				iterator.disable_recursion_pending();
				iterator.increment(errorCode);
				if (!RemoveGeneratedPath(path, errorMessage))
				{
					return false;
				}
				continue;
			}

			iterator.increment(errorCode);
		}
		if (errorCode)
		{
			errorMessage = "Failed to enumerate project artifacts: " + projectsDirectory.string() + ": " + errorCode.message();
			return false;
		}

		return true;
	}

	bool RequiresNativeBuildOutputReset(BuildFilesFreshnessState state)
	{
		return state == BuildFilesFreshnessState::GeneratorMismatch || state == BuildFilesFreshnessState::FreshnessStampMismatch;
	}

	bool ResetNativeBuildOutputs(
	    const std::filesystem::path& repositoryRoot,
	    const std::filesystem::path& buildDirectory,
	    std::string& errorMessage)
	{
		errorMessage.clear();
		if (!ValidateBuildDirectoryResetScope(repositoryRoot, buildDirectory, errorMessage))
		{
			return false;
		}
		if (!RemoveGeneratedBuildTree(buildDirectory, errorMessage))
		{
			return false;
		}

		const std::filesystem::path developerArtifacts = GetDeveloperArtifactDirectory(repositoryRoot);
		const std::array<std::filesystem::path, 5> compiledArtifactRoots = {
		    developerArtifacts / "launcher",
		    developerArtifacts / "libraries",
		    developerArtifacts / "runtime-support",
		    developerArtifacts / "tools",
		    GetSymbolDirectory(repositoryRoot),
		};
		for (const std::filesystem::path& path : compiledArtifactRoots)
		{
			if (!RemoveGeneratedPath(path, errorMessage))
			{
				return false;
			}
		}

		return RemoveProjectBuildOutputs(developerArtifacts / "projects", errorMessage);
	}
}

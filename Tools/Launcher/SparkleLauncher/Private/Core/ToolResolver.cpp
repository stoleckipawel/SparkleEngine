#include "SparkleLauncher/ToolResolver.h"

#include "SparkleLauncher/LauncherPaths.h"

#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <optional>
#include <sstream>
#include <system_error>

namespace SparkleLauncher
{
	static std::optional<std::string> TryGetEnvironmentVariable(const char* name)
	{
		if (name == nullptr)
		{
			return std::nullopt;
		}

#if defined(_MSC_VER)
		char* rawValue = nullptr;
		size_t requiredLength = 0;
		if (_dupenv_s(&rawValue, &requiredLength, name) != 0 || rawValue == nullptr || requiredLength <= 1)
		{
			if (rawValue != nullptr)
			{
				std::free(rawValue);
			}
			return std::nullopt;
		}

		std::string value(rawValue, requiredLength - 1);
		std::free(rawValue);
		return value;
#else
		const char* value = std::getenv(name);
		if (value == nullptr || value[0] == '\0')
		{
			return std::nullopt;
		}
		return std::string(value);
#endif
	}

	static std::vector<std::string> GetCandidateExecutableNames(KnownTool tool)
	{
		switch (tool)
		{
		case KnownTool::CMake:
			return {"cmake.exe", "cmake"};
		case KnownTool::MSBuild:
			return {"MSBuild.exe", "msbuild.exe", "MSBuild"};
		case KnownTool::Rider:
			return {"rider64.exe", "rider.exe", "rider.bat", "rider"};
		case KnownTool::Git:
			return {"git.exe", "git"};
		case KnownTool::ClangFormat:
			return {"clang-format.exe", "clang-format"};
		}

		return {};
	}

	static std::vector<std::string> GetToolOverrideEnvironmentNames(KnownTool tool)
	{
		switch (tool)
		{
		case KnownTool::CMake:
			return {"SPARKLE_CMAKE_EXE"};
		case KnownTool::MSBuild:
			return {"SPARKLE_MSBUILD_EXE"};
		case KnownTool::Rider:
			return {"SPARKLE_RIDER_EXE"};
		case KnownTool::Git:
			return {"SPARKLE_GIT_EXE"};
		case KnownTool::ClangFormat:
			return {"SPARKLE_CLANG_FORMAT_EXE"};
		}

		return {};
	}

	static void AddProgramFilesCandidate(std::vector<std::filesystem::path>& candidates, const char* environmentName, std::filesystem::path relativePath)
	{
		const std::optional<std::string> root = TryGetEnvironmentVariable(environmentName);
		if (root.has_value())
		{
			candidates.push_back(std::filesystem::path(*root) / relativePath);
		}
	}

	static void AddProgramFilesDirectoryMatches(
	    std::vector<std::filesystem::path>& candidates,
	    const char* environmentName,
	    std::string_view directoryPrefix,
	    std::filesystem::path relativeExecutablePath)
	{
		const std::optional<std::string> root = TryGetEnvironmentVariable(environmentName);
		if (!root.has_value())
		{
			return;
		}

		const std::filesystem::path parentDirectory = std::filesystem::path(*root) / "JetBrains";
		std::error_code errorCode;
		if (!std::filesystem::is_directory(parentDirectory, errorCode))
		{
			return;
		}

		for (const std::filesystem::directory_entry& entry : std::filesystem::directory_iterator(parentDirectory, errorCode))
		{
			if (errorCode)
			{
				errorCode.clear();
				continue;
			}

			if (!entry.is_directory(errorCode))
			{
				errorCode.clear();
				continue;
			}

			const std::string directoryName = entry.path().filename().string();
			if (directoryName.rfind(std::string(directoryPrefix), 0) == 0)
			{
				candidates.push_back(entry.path() / relativeExecutablePath);
			}
			errorCode.clear();
		}
	}

	static void AddLocalAppDataCandidate(std::vector<std::filesystem::path>& candidates, std::filesystem::path relativePath)
	{
		const std::optional<std::string> root = TryGetEnvironmentVariable("LocalAppData");
		if (root.has_value())
		{
			candidates.push_back(std::filesystem::path(*root) / relativePath);
		}
	}

	static void AddEnvironmentRootIfPresent(std::vector<std::filesystem::path>& roots, const char* environmentName, std::filesystem::path relativePath = {})
	{
		const std::optional<std::string> root = TryGetEnvironmentVariable(environmentName);
		if (root.has_value())
		{
			roots.push_back(std::filesystem::path(*root) / relativePath);
		}
	}

	static bool PathFilenameEquals(const std::filesystem::path& path, std::string_view filename)
	{
		std::string pathFilename = path.filename().string();
		std::string expected(filename);
		std::transform(pathFilename.begin(), pathFilename.end(), pathFilename.begin(), [](unsigned char value) { return static_cast<char>(std::tolower(value)); });
		std::transform(expected.begin(), expected.end(), expected.begin(), [](unsigned char value) { return static_cast<char>(std::tolower(value)); });
		return pathFilename == expected;
	}

	static void AddRecursiveExecutableMatches(
	    std::vector<std::filesystem::path>& candidates,
	    const std::filesystem::path& root,
	    std::string_view filename,
	    int maxDepth)
	{
		std::error_code errorCode;
		if (!std::filesystem::is_directory(root, errorCode))
		{
			return;
		}

		std::filesystem::recursive_directory_iterator iterator(
		    root,
		    std::filesystem::directory_options::skip_permission_denied,
		    errorCode);
		const std::filesystem::recursive_directory_iterator end;
		while (iterator != end)
		{
			if (errorCode)
			{
				errorCode.clear();
				iterator.increment(errorCode);
				continue;
			}

			if (iterator.depth() > maxDepth)
			{
				iterator.disable_recursion_pending();
			}

			const std::filesystem::directory_entry entry = *iterator;
			if (entry.is_regular_file(errorCode) && PathFilenameEquals(entry.path(), filename))
			{
				candidates.push_back(entry.path());
			}
			errorCode.clear();
			iterator.increment(errorCode);
		}
	}

	static void AddVisualStudioMSBuildCandidates(std::vector<std::filesystem::path>& candidates)
	{
		const std::vector<std::string> versions = {"18", "2022"};
		const std::vector<std::string> editions = {"Community", "Professional", "Enterprise", "BuildTools"};
		for (const std::string& version : versions)
		{
			for (const std::string& edition : editions)
			{
				AddProgramFilesCandidate(
				    candidates,
				    "ProgramFiles",
				    std::filesystem::path("Microsoft Visual Studio") / version / edition / "MSBuild" / "Current" / "Bin" / "MSBuild.exe");
			}
		}
	}

	static void AddJetBrainsRiderCandidates(std::vector<std::filesystem::path>& candidates)
	{
		AddLocalAppDataCandidate(candidates, std::filesystem::path("Programs") / "Rider" / "bin" / "rider64.exe");
		AddLocalAppDataCandidate(candidates, std::filesystem::path("JetBrains") / "Toolbox" / "scripts" / "rider.cmd");
		AddProgramFilesCandidate(candidates, "ProgramFiles", std::filesystem::path("JetBrains") / "JetBrains Rider" / "bin" / "rider64.exe");
		AddProgramFilesCandidate(candidates, "ProgramFiles(x86)", std::filesystem::path("JetBrains") / "JetBrains Rider" / "bin" / "rider64.exe");
		AddProgramFilesDirectoryMatches(candidates, "ProgramFiles", "JetBrains Rider", std::filesystem::path("bin") / "rider64.exe");
		AddProgramFilesDirectoryMatches(candidates, "ProgramFiles(x86)", "JetBrains Rider", std::filesystem::path("bin") / "rider64.exe");

		std::vector<std::filesystem::path> searchRoots;
		AddEnvironmentRootIfPresent(searchRoots, "ProgramFiles", std::filesystem::path("JetBrains"));
		AddEnvironmentRootIfPresent(searchRoots, "ProgramFiles(x86)", std::filesystem::path("JetBrains"));
		AddEnvironmentRootIfPresent(searchRoots, "LocalAppData", std::filesystem::path("Programs"));
		AddEnvironmentRootIfPresent(searchRoots, "LocalAppData", std::filesystem::path("JetBrains"));
		AddEnvironmentRootIfPresent(searchRoots, "LocalAppData", std::filesystem::path("JetBrains") / "Toolbox");

		for (const std::filesystem::path& root : searchRoots)
		{
			AddRecursiveExecutableMatches(candidates, root, "rider64.exe", 4);
			AddRecursiveExecutableMatches(candidates, root, "rider.exe", 4);
			AddRecursiveExecutableMatches(candidates, root, "rider.cmd", 5);
		}
	}

	static std::vector<std::filesystem::path> GetKnownInstallCandidates(KnownTool tool)
	{
		std::vector<std::filesystem::path> candidates;
		for (const std::string& environmentName : GetToolOverrideEnvironmentNames(tool))
		{
			const std::optional<std::string> overridePath = TryGetEnvironmentVariable(environmentName.c_str());
			if (overridePath.has_value())
			{
				candidates.emplace_back(*overridePath);
			}
		}

		switch (tool)
		{
		case KnownTool::CMake:
			AddProgramFilesCandidate(candidates, "ProgramFiles", std::filesystem::path("CMake") / "bin" / "cmake.exe");
			break;
		case KnownTool::MSBuild:
			AddVisualStudioMSBuildCandidates(candidates);
			break;
		case KnownTool::Rider:
			AddJetBrainsRiderCandidates(candidates);
			break;
		case KnownTool::Git:
			AddProgramFilesCandidate(candidates, "ProgramFiles", std::filesystem::path("Git") / "cmd" / "git.exe");
			break;
		case KnownTool::ClangFormat:
			AddProgramFilesCandidate(candidates, "ProgramFiles", std::filesystem::path("LLVM") / "bin" / "clang-format.exe");
			break;
		}

		return candidates;
	}

	static std::optional<std::filesystem::path> FindExistingExecutable(std::vector<std::filesystem::path> candidates)
	{
		std::error_code errorCode;
		for (const std::filesystem::path& candidate : candidates)
		{
			if (std::filesystem::exists(candidate, errorCode) && std::filesystem::is_regular_file(candidate, errorCode))
			{
				return candidate;
			}
			errorCode.clear();
		}
		return std::nullopt;
	}

	static std::filesystem::path FirstExistingOrPreferred(const std::vector<std::filesystem::path>& candidates)
	{
		std::error_code errorCode;
		for (const std::filesystem::path& candidate : candidates)
		{
			if (std::filesystem::exists(candidate, errorCode) && std::filesystem::is_regular_file(candidate, errorCode))
			{
				return candidate;
			}
			errorCode.clear();
		}
		return candidates.empty() ? std::filesystem::path() : candidates.front();
	}

	static bool IsKnownDevelopmentTool(std::string_view executableName)
	{
		return executableName == "AssetCooker" || executableName == "TextureCooker" || executableName == "ShaderCompiler" || executableName == "AssetConverter";
	}

	std::string ToString(KnownTool tool)
	{
		switch (tool)
		{
		case KnownTool::CMake:
			return "CMake";
		case KnownTool::MSBuild:
			return "MSBuild";
		case KnownTool::Rider:
			return "Rider";
		case KnownTool::Git:
			return "Git";
		case KnownTool::ClangFormat:
			return "clang-format";
		}

		return "Unknown";
	}

	std::vector<std::filesystem::path> GetExecutableSearchPath()
	{
		std::vector<std::filesystem::path> searchPaths;
		const std::optional<std::string> pathValue = TryGetEnvironmentVariable("PATH");
		if (!pathValue.has_value())
		{
			return searchPaths;
		}

#if defined(_WIN32)
		constexpr char separator = ';';
#else
		constexpr char separator = ':';
#endif
		std::stringstream stream(*pathValue);
		std::string segment;
		while (std::getline(stream, segment, separator))
		{
			if (!segment.empty())
			{
				searchPaths.emplace_back(segment);
			}
		}

		return searchPaths;
	}

	std::optional<std::filesystem::path> FindExecutableOnPath(std::string_view executableName)
	{
		std::error_code errorCode;
		for (const std::filesystem::path& directory : GetExecutableSearchPath())
		{
			const std::filesystem::path candidate = directory / std::string(executableName);
			if (std::filesystem::exists(candidate, errorCode) && std::filesystem::is_regular_file(candidate, errorCode))
			{
				return candidate;
			}
		}

		return std::nullopt;
	}

	ToolResolveResult ResolveKnownTool(KnownTool tool)
	{
		ToolResolveResult result;
		result.Tool = tool;
		result.DisplayName = ToString(tool);

		for (const std::string& candidateName : GetCandidateExecutableNames(tool))
		{
			if (const std::optional<std::filesystem::path> path = FindExecutableOnPath(candidateName))
			{
				result.Found = true;
				result.Path = *path;
				return result;
			}
		}

		if (const std::optional<std::filesystem::path> path = FindExistingExecutable(GetKnownInstallCandidates(tool)))
		{
			result.Found = true;
			result.Path = *path;
			return result;
		}

		result.FailureReason = result.DisplayName + " executable was not found on PATH or in known install locations.";
		return result;
	}

	std::filesystem::path ResolveSparkleToolPath(
	    const std::filesystem::path& repositoryRoot,
	    std::string_view profileName,
	    std::string_view executableName)
	{
		std::filesystem::path fileName(executableName);
#if defined(_WIN32)
		if (fileName.extension().empty())
		{
			fileName += ".exe";
		}
#endif
		const std::filesystem::path legacyPath = GetBuildBinaryDirectory(repositoryRoot, profileName) / fileName;
		if (executableName == "SparkleLauncher" || executableName == "SparkleLauncherProbe")
		{
			return FirstExistingOrPreferred({
			    GetLauncherArtifactDirectory(repositoryRoot, profileName) / fileName,
			    legacyPath,
			});
		}
		if (IsKnownDevelopmentTool(executableName))
		{
			return FirstExistingOrPreferred({
			    GetDevelopmentToolArtifactDirectory(repositoryRoot, executableName, profileName) / fileName,
			    legacyPath,
			});
		}
		return legacyPath;
	}
}

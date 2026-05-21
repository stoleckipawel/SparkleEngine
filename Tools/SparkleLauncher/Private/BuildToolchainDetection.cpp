#include "SparkleLauncher/BuildWorkspaceOperations.h"

#include "BuildWorkspaceStateFiles.h"
#include "Core/Public/Environment/EnvironmentVariables.h"
#include "Core/Public/Strings/StringUtils.h"
#include "SparkleLauncher/LauncherPaths.h"
#include "SparkleLauncher/ToolResolver.h"

#include <algorithm>
#include <optional>
#include <system_error>
#include <utility>

namespace SparkleLauncher
{
	static constexpr std::string_view kVisualStudioCppComponent = "Microsoft.VisualStudio.Component.VC.Tools.x86.x64";
	static constexpr std::string_view kMinimumCMakeVersion = "3.20.0";
	static constexpr std::string_view kMinimumGitVersion = "2.25.0";

	static std::optional<std::filesystem::path> ResolveProgramFilesX86()
	{
		std::string value;
		if (Environment::TryGetVariable("ProgramFiles(x86)", value))
		{
			return std::filesystem::path(value);
		}
		if (Environment::TryGetVariable("ProgramFiles", value))
		{
			return std::filesystem::path(value);
		}
		return std::nullopt;
	}

	static std::optional<std::filesystem::path> ResolveVswherePath()
	{
		std::string overridePath;
		if (Environment::TryGetVariable("SPARKLE_VSWHERE_EXE", overridePath))
		{
			std::filesystem::path path(overridePath);
			std::error_code errorCode;
			if (std::filesystem::exists(path, errorCode))
			{
				return path;
			}
		}

		const std::optional<std::filesystem::path> programFiles = ResolveProgramFilesX86();
		if (!programFiles.has_value())
		{
			return std::nullopt;
		}

		const std::filesystem::path path = *programFiles / "Microsoft Visual Studio" / "Installer" / "vswhere.exe";
		std::error_code errorCode;
		return std::filesystem::exists(path, errorCode) ? std::optional<std::filesystem::path>(path) : std::nullopt;
	}

	static std::optional<std::string> FindWindowsSdkVersion()
	{
		const std::optional<std::filesystem::path> programFiles = ResolveProgramFilesX86();
		if (!programFiles.has_value())
		{
			return std::nullopt;
		}

		const std::filesystem::path includeRoot = *programFiles / "Windows Kits" / "10" / "Include";
		std::error_code errorCode;
		if (!std::filesystem::is_directory(includeRoot, errorCode))
		{
			return std::nullopt;
		}

		std::optional<std::string> latestVersion;
		for (const std::filesystem::directory_entry& entry : std::filesystem::directory_iterator(includeRoot, errorCode))
		{
			if (!entry.is_directory(errorCode))
			{
				continue;
			}

			const std::string version = entry.path().filename().string();
			if (!latestVersion.has_value() || version > *latestVersion)
			{
				latestVersion = version;
			}
		}

		return latestVersion;
	}

	static std::string ResolveGenerator(const std::filesystem::path& repositoryRoot)
	{
		std::string overrideGenerator;
		if (Environment::TryGetVariable("SPARKLE_CMAKE_GENERATOR", overrideGenerator))
		{
			return overrideGenerator;
		}

		const std::filesystem::path cachePath = GetBuildDirectory(repositoryRoot) / "CMakeCache.txt";
		if (const std::optional<std::string> cacheGenerator = ReadCMakeCacheValue(cachePath, "CMAKE_GENERATOR"))
		{
			return *cacheGenerator;
		}

		const std::filesystem::path stampPath = GetBuildDirectory(repositoryRoot) / "BuildFilesFreshness.json";
		if (const std::optional<std::string> stampedGenerator = ReadBuildFilesFreshnessStampValue(stampPath, "generator"))
		{
			return *stampedGenerator;
		}

		const std::optional<std::filesystem::path> programFiles = ResolveProgramFilesX86();
		if (programFiles.has_value())
		{
			std::error_code errorCode;
			if (std::filesystem::exists(*programFiles / "Microsoft Visual Studio" / "2026", errorCode))
			{
				return "Visual Studio 18 2026";
			}
		}

		return "Visual Studio 17 2022";
	}

	static std::string ResolvePlatform()
	{
		std::string architecture;
		return Environment::TryGetVariable("SPARKLE_CMAKE_ARCH", architecture) ? architecture : "x64";
	}

	static std::string ResolveToolset()
	{
		std::string toolset;
		if (Environment::TryGetVariable("SPARKLE_CMAKE_TOOLSET", toolset))
		{
			return toolset;
		}
		if (Environment::GetFlag("SPARKLE_USE_CLANGCL"))
		{
			return "ClangCL";
		}
		return {};
	}

	static ToolchainItemStatus MakeToolStatus(
	    std::string id,
	    std::string displayName,
	    bool required,
	    bool found,
	    std::filesystem::path path,
	    std::string detail)
	{
		ToolchainItemStatus status;
		status.Id = std::move(id);
		status.DisplayName = std::move(displayName);
		status.Required = required;
		status.State = found ? ToolchainItemState::Found : (required ? ToolchainItemState::Missing : ToolchainItemState::Warning);
		status.Path = std::move(path);
		status.Detail = std::move(detail);
		return status;
	}

	static void AppendKnownToolStatus(BuildToolchainStatus& status, KnownTool tool, bool required, std::string detail)
	{
		const ToolResolveResult resolvedTool = ResolveKnownTool(tool);
		status.Items.push_back(MakeToolStatus(
		    Strings::ToLowerCopy(ToString(tool)),
		    ToString(tool),
		    required,
		    resolvedTool.Found,
		    resolvedTool.Path,
		    resolvedTool.Found ? std::move(detail) : resolvedTool.FailureReason));

		switch (tool)
		{
		case KnownTool::CMake:
			status.CMakePath = resolvedTool.Path;
			break;
		case KnownTool::MSBuild:
			status.MSBuildPath = resolvedTool.Path;
			break;
		case KnownTool::Git:
			status.GitPath = resolvedTool.Path;
			break;
		case KnownTool::ClangFormat:
			status.ClangFormatPath = resolvedTool.Path;
			break;
		}
	}

	static bool AreRequiredToolsAvailable(const std::vector<ToolchainItemStatus>& items)
	{
		return std::none_of(items.begin(), items.end(), [](const ToolchainItemStatus& item) {
			return item.Required && item.State != ToolchainItemState::Found;
		});
	}

	BuildToolchainStatus DetectBuildToolchain(const std::filesystem::path& repositoryRoot)
	{
		BuildToolchainStatus status;
		status.Generator = ResolveGenerator(repositoryRoot);
		status.Platform = ResolvePlatform();
		status.Toolset = ResolveToolset();

		AppendKnownToolStatus(status, KnownTool::CMake, true, "Minimum required version: " + std::string(kMinimumCMakeVersion));
		AppendKnownToolStatus(status, KnownTool::MSBuild, true, "Required by Visual Studio CMake builds.");
		AppendKnownToolStatus(status, KnownTool::Git, true, "Minimum required version: " + std::string(kMinimumGitVersion));
		AppendKnownToolStatus(status, KnownTool::ClangFormat, false, "Required only for format operations.");

		status.VswherePath = ResolveVswherePath().value_or(std::filesystem::path());
		status.Items.push_back(MakeToolStatus(
		    "visualstudio",
		    "Visual Studio C++ tools",
		    true,
		    !status.VswherePath.empty(),
		    status.VswherePath,
		    !status.VswherePath.empty() ? "vswhere is available for C++ workload discovery: " + std::string(kVisualStudioCppComponent) : "vswhere was not found."));

		status.WindowsSdkVersion = FindWindowsSdkVersion().value_or(std::string());
		status.Items.push_back(MakeToolStatus(
		    "windowssdk",
		    "Windows SDK",
		    true,
		    !status.WindowsSdkVersion.empty(),
		    {},
		    !status.WindowsSdkVersion.empty() ? "Latest SDK: " + status.WindowsSdkVersion : "Windows Kits 10 Include directory was not found."));

		const bool clangClRequired = status.Toolset == "ClangCL";
		const std::optional<std::filesystem::path> clangClPath = FindExecutableOnPath("clang-cl.exe");
		status.Items.push_back(MakeToolStatus(
		    "clangcl",
		    "clang-cl",
		    clangClRequired,
		    clangClPath.has_value(),
		    clangClPath.value_or(std::filesystem::path()),
		    clangClRequired ? "Required by selected CMake toolset ClangCL." : "Optional; set SPARKLE_USE_CLANGCL=1 to request ClangCL."));

		status.RequiredToolsAvailable = AreRequiredToolsAvailable(status.Items);
		return status;
	}
}

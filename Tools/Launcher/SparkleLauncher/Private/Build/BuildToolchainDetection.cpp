#include "SparkleLauncher/BuildWorkspaceOperations.h"

#include "BuildWorkspaceStateFiles.h"
#include "Core/Public/Environment/EnvironmentVariables.h"
#include "Core/Public/Strings/StringUtils.h"
#include "SparkleLauncher/LauncherPaths.h"
#include "SparkleLauncher/ToolResolver.h"

#include <algorithm>
#include <array>
#include <cctype>
#include <optional>
#include <sstream>
#include <system_error>
#include <utility>

namespace SparkleLauncher
{
	static constexpr std::string_view kVisualStudioCppComponent = "Microsoft.VisualStudio.Component.VC.Tools.x86.x64";
	static constexpr std::string_view kMinimumCMakeVersion = "3.20.0";
	static constexpr std::string_view kMinimumGitVersion = "2.25.0";

	static bool IsVisualStudioGenerator(std::string_view generator)
	{
		return generator.find("Visual Studio") != std::string_view::npos;
	}

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

	static std::string ToLower(std::string value)
	{
		std::transform(value.begin(), value.end(), value.begin(), [](unsigned char character) {
			return static_cast<char>(std::tolower(character));
		});
		return value;
	}

	static std::string BuildPathSortKey(const std::filesystem::path& path)
	{
		return ToLower(path.lexically_normal().generic_string());
	}

	static bool PathLooksLikeQtMsvcKitRoot(const std::filesystem::path& path)
	{
		const std::string directoryName = ToLower(path.filename().string());
		return directoryName.find("msvc") != std::string::npos && directoryName.find("64") != std::string::npos && directoryName.find("arm64") == std::string::npos;
	}

	static bool PathLooksLikeQtMingwKitRoot(const std::filesystem::path& path)
	{
		const std::string directoryName = ToLower(path.filename().string());
		return directoryName.find("mingw") != std::string::npos;
	}

	static std::optional<std::filesystem::path> TryGetEnvironmentPath(std::string_view variableName)
	{
		std::string value;
		const std::string variableNameText(variableName);
		if (!Environment::TryGetVariable(variableNameText.c_str(), value) || value.empty())
		{
			return std::nullopt;
		}
		return std::filesystem::path(value);
	}

	static std::optional<std::filesystem::path> FindLatestVersionedDirectory(const std::filesystem::path& root)
	{
		std::error_code errorCode;
		if (!std::filesystem::is_directory(root, errorCode))
		{
			return std::nullopt;
		}

		std::optional<std::filesystem::path> latestDirectory;
		for (const std::filesystem::directory_entry& entry : std::filesystem::directory_iterator(root, errorCode))
		{
			if (!entry.is_directory(errorCode))
			{
				errorCode.clear();
				continue;
			}

			if (!latestDirectory.has_value() || BuildPathSortKey(latestDirectory->filename()) < BuildPathSortKey(entry.path().filename()))
			{
				latestDirectory = entry.path();
			}
			errorCode.clear();
		}

		return latestDirectory;
	}

	static std::optional<std::filesystem::path> DetectInstalledVulkanSdkRoot()
	{
		if (const std::optional<std::filesystem::path> envRoot = TryGetEnvironmentPath("VULKAN_SDK"))
		{
			return envRoot->lexically_normal();
		}
		if (const std::optional<std::filesystem::path> envRoot = TryGetEnvironmentPath("VK_SDK_PATH"))
		{
			return envRoot->lexically_normal();
		}

		const std::array<std::filesystem::path, 3> candidateRoots = {
		    std::filesystem::path("C:\\VulkanSDK"),
		    std::filesystem::path("C:\\Program Files\\VulkanSDK"),
		    std::filesystem::path("C:\\Program Files (x86)\\VulkanSDK"),
		};

		for (const std::filesystem::path& candidateRoot : candidateRoots)
		{
			if (const std::optional<std::filesystem::path> versionRoot = FindLatestVersionedDirectory(candidateRoot))
			{
				return versionRoot->lexically_normal();
			}
		}

		return std::nullopt;
	}

	static std::optional<std::filesystem::path> NormalizeQtKitRootCandidate(std::filesystem::path path)
	{
		std::error_code errorCode;
		if (path.empty())
		{
			return std::nullopt;
		}

		path = path.lexically_normal();
		if (std::filesystem::is_regular_file(path, errorCode))
		{
			if (ToLower(path.filename().string()) == "qmake.exe")
			{
				const std::filesystem::path root = path.parent_path().parent_path();
				return std::filesystem::exists(root / "lib" / "cmake" / "Qt6" / "Qt6Config.cmake", errorCode) ? std::optional<std::filesystem::path>(root) : std::nullopt;
			}
			return std::nullopt;
		}
		errorCode.clear();

		if (!std::filesystem::is_directory(path, errorCode))
		{
			return std::nullopt;
		}
		errorCode.clear();

		if (std::filesystem::exists(path / "lib" / "cmake" / "Qt6" / "Qt6Config.cmake", errorCode))
		{
			return path;
		}
		errorCode.clear();

		if (ToLower(path.filename().string()) == "qt6")
		{
			const std::filesystem::path cmakeRoot = path.parent_path().parent_path();
			if (std::filesystem::exists(cmakeRoot / "bin" / "qmake.exe", errorCode) &&
			    std::filesystem::exists(cmakeRoot / "lib" / "cmake" / "Qt6" / "Qt6Config.cmake", errorCode))
			{
				return cmakeRoot;
			}
		}

		return std::nullopt;
	}

	static std::vector<std::filesystem::path> SplitPathList(std::string_view value)
	{
		std::vector<std::filesystem::path> paths;
		std::stringstream stream{std::string(value)};
		std::string segment;
		while (std::getline(stream, segment, ';'))
		{
			if (!segment.empty())
			{
				paths.emplace_back(segment);
			}
		}
		return paths;
	}

	struct QtKitDiscovery
	{
		bool FoundMsvcKit = false;
		std::filesystem::path QtRootPath;
		std::filesystem::path QtQmakePath;
		std::vector<std::filesystem::path> MsvcCandidates;
		std::vector<std::filesystem::path> MingwCandidates;
	};

	static void AddQtKitCandidate(
	    QtKitDiscovery& discovery,
	    const std::filesystem::path& candidate,
	    std::vector<std::filesystem::path>& seenCandidates)
	{
		const std::optional<std::filesystem::path> normalizedRoot = NormalizeQtKitRootCandidate(candidate);
		if (!normalizedRoot.has_value())
		{
			return;
		}

		const std::filesystem::path root = normalizedRoot->lexically_normal();
		if (std::find(seenCandidates.begin(), seenCandidates.end(), root) != seenCandidates.end())
		{
			return;
		}
		seenCandidates.push_back(root);

		if (PathLooksLikeQtMsvcKitRoot(root))
		{
			discovery.MsvcCandidates.push_back(root);
			return;
		}

		if (PathLooksLikeQtMingwKitRoot(root))
		{
			discovery.MingwCandidates.push_back(root);
		}
	}

	static void FinalizeQtKitDiscovery(QtKitDiscovery& discovery)
	{
		const auto byPath = [](const std::filesystem::path& left, const std::filesystem::path& right)
		{
			return BuildPathSortKey(left) < BuildPathSortKey(right);
		};
		std::ranges::sort(discovery.MsvcCandidates, byPath);
		std::ranges::sort(discovery.MingwCandidates, byPath);

		discovery.FoundMsvcKit = !discovery.MsvcCandidates.empty();
		discovery.QtRootPath = discovery.FoundMsvcKit ? discovery.MsvcCandidates.front() : std::filesystem::path();
		discovery.QtQmakePath = discovery.FoundMsvcKit ? discovery.QtRootPath / "bin" / "qmake.exe" : std::filesystem::path();
	}

	static QtKitDiscovery DiscoverQtKit()
	{
		QtKitDiscovery discovery;
		std::vector<std::filesystem::path> seenCandidates;

		std::string value;
		if (Environment::TryGetVariable("SPARKLE_QT_ROOT", value))
		{
			AddQtKitCandidate(discovery, value, seenCandidates);
		}
		if (Environment::TryGetVariable("QTDIR", value))
		{
			AddQtKitCandidate(discovery, value, seenCandidates);
		}
		if (Environment::TryGetVariable("Qt6_DIR", value))
		{
			AddQtKitCandidate(discovery, value, seenCandidates);
		}
		if (Environment::TryGetVariable("CMAKE_PREFIX_PATH", value))
		{
			for (const std::filesystem::path& path : SplitPathList(value))
			{
				AddQtKitCandidate(discovery, path, seenCandidates);
			}
		}

		const std::filesystem::path qtRoot("C:\\Qt");
		std::error_code errorCode;
		if (std::filesystem::is_directory(qtRoot, errorCode))
		{
			for (const std::filesystem::directory_entry& versionEntry : std::filesystem::directory_iterator(qtRoot, errorCode))
			{
				if (errorCode)
				{
					errorCode.clear();
					continue;
				}
				if (!versionEntry.is_directory(errorCode))
				{
					errorCode.clear();
					continue;
				}

				for (const std::filesystem::directory_entry& kitEntry : std::filesystem::directory_iterator(versionEntry.path(), errorCode))
				{
					if (errorCode)
					{
						errorCode.clear();
						continue;
					}
					AddQtKitCandidate(discovery, kitEntry.path(), seenCandidates);
				}
			}
		}

		FinalizeQtKitDiscovery(discovery);
		return discovery;
	}

	static std::string BuildQtStatusDetail(const QtKitDiscovery& discovery)
	{
		if (discovery.FoundMsvcKit)
		{
			return "Using Qt MSVC kit root: " + discovery.QtRootPath.string();
		}

		if (!discovery.MingwCandidates.empty())
		{
			return "Only MinGW Qt kits were found. Install a Qt 6 MSVC x64 kit and set SPARKLE_QT_ROOT if needed.";
		}

		return "No Qt 6 MSVC x64 kit was found. Install one under C:\\Qt or set SPARKLE_QT_ROOT to the kit root.";
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

	struct ShaderCompilerSdkStatus
	{
		bool Available = false;
		std::filesystem::path Root;
		std::string Detail;
	};

	struct VulkanSdkStatus
	{
		bool Available = false;
		std::filesystem::path Root;
		std::string Detail;
	};

	static ShaderCompilerSdkStatus DetectShaderCompilerSdk()
	{
		ShaderCompilerSdkStatus status;
		const std::optional<std::filesystem::path> sdkRoot = DetectInstalledVulkanSdkRoot();
		if (!sdkRoot.has_value())
		{
			status.Detail = "Vulkan SDK was not detected. Install it or define VULKAN_SDK so the enabled ShaderCompiler workspace tier can find DXC and Slang.";
			return status;
		}

		status.Root = sdkRoot->lexically_normal();
		const std::filesystem::path dxcHeader = status.Root / "Include" / "dxc" / "dxcapi.h";
		const std::filesystem::path dxcLibrary = status.Root / "Lib" / "dxcompiler.lib";
		const std::filesystem::path slangHeader = status.Root / "Include" / "slang" / "slang.h";
		const std::filesystem::path slangLibrary = status.Root / "Lib" / "slang.lib";
		const std::filesystem::path slangRuntime = status.Root / "Bin" / "slang.dll";

		std::vector<std::string> missingEntries;
		std::error_code errorCode;
		if (!std::filesystem::exists(dxcHeader, errorCode))
		{
			missingEntries.push_back("Include/dxc/dxcapi.h");
		}
		errorCode.clear();
		if (!std::filesystem::exists(dxcLibrary, errorCode))
		{
			missingEntries.push_back("Lib/dxcompiler.lib");
		}
		errorCode.clear();
		if (!std::filesystem::exists(slangHeader, errorCode))
		{
			missingEntries.push_back("Include/slang/slang.h");
		}
		errorCode.clear();
		if (!std::filesystem::exists(slangLibrary, errorCode))
		{
			missingEntries.push_back("Lib/slang.lib");
		}
		errorCode.clear();
		if (!std::filesystem::exists(slangRuntime, errorCode))
		{
			missingEntries.push_back("Bin/slang.dll");
		}

		if (missingEntries.empty())
		{
			status.Available = true;
			status.Detail = "Using VULKAN_SDK root: " + status.Root.string();
			return status;
		}

		std::vector<std::string_view> missingEntryViews;
		missingEntryViews.reserve(missingEntries.size());
		for (const std::string& entry : missingEntries)
		{
			missingEntryViews.push_back(entry);
		}

		std::ostringstream detail;
		detail << "Detected Vulkan SDK root " << status.Root.string()
		       << ", but the enabled ShaderCompiler workspace tier is missing: "
		       << Strings::Join(missingEntryViews, ", ")
		       << ".";
		status.Detail = detail.str();
		return status;
	}

	static VulkanSdkStatus DetectVulkanSdk()
	{
		VulkanSdkStatus status;
		const std::optional<std::filesystem::path> sdkRoot = DetectInstalledVulkanSdkRoot();
		if (!sdkRoot.has_value())
		{
			status.Detail = "Vulkan SDK was not detected. Install it or define VULKAN_SDK so Vulkan and NVIDIA Streamline builds can resolve Vulkan headers and import libraries.";
			return status;
		}

		status.Root = sdkRoot->lexically_normal();
		const std::filesystem::path vulkanHeader = status.Root / "Include" / "vulkan" / "vulkan.h";
		const std::filesystem::path vulkanLibrary = status.Root / "Lib" / "vulkan-1.lib";

		std::vector<std::string> missingEntries;
		std::error_code errorCode;
		if (!std::filesystem::exists(vulkanHeader, errorCode))
		{
			missingEntries.push_back("Include/vulkan/vulkan.h");
		}
		errorCode.clear();
		if (!std::filesystem::exists(vulkanLibrary, errorCode))
		{
			missingEntries.push_back("Lib/vulkan-1.lib");
		}

		if (missingEntries.empty())
		{
			status.Available = true;
			status.Detail = "Using VULKAN_SDK root: " + status.Root.string();
			return status;
		}

		std::vector<std::string_view> missingEntryViews;
		missingEntryViews.reserve(missingEntries.size());
		for (const std::string& entry : missingEntries)
		{
			missingEntryViews.push_back(entry);
		}

		std::ostringstream detail;
		detail << "Detected Vulkan SDK root " << status.Root.string()
		       << ", but required Vulkan SDK files are missing: "
		       << Strings::Join(missingEntryViews, ", ")
		       << ".";
		status.Detail = detail.str();
		return status;
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
			const std::filesystem::path visualStudioRoot = *programFiles / "Microsoft Visual Studio";
			if (std::filesystem::exists(visualStudioRoot / "18", errorCode) ||
			    std::filesystem::exists(visualStudioRoot / "2026", errorCode))
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
		case KnownTool::Rider:
			status.RiderPath = resolvedTool.Path;
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

	BuildToolchainStatus DetectBuildToolchain(const std::filesystem::path& repositoryRoot, WorkspaceIde preferredIde)
	{
		BuildToolchainStatus status;
		const WorkspaceFeatureSettings featureSettings = GetLauncherWorkspaceFeatureSettings();
		status.Generator = ResolveGenerator(repositoryRoot);
		status.Platform = ResolvePlatform();
		status.Toolset = ResolveToolset();
		const bool visualStudioGenerator = IsVisualStudioGenerator(status.Generator);

		AppendKnownToolStatus(status, KnownTool::CMake, true, "Minimum required version: " + std::string(kMinimumCMakeVersion));
		AppendKnownToolStatus(status, KnownTool::MSBuild, visualStudioGenerator, visualStudioGenerator ? "Required by the selected CMake generator." : "Optional for non-Visual Studio generators.");
		AppendKnownToolStatus(status, KnownTool::Rider, preferredIde == WorkspaceIde::Rider, preferredIde == WorkspaceIde::Rider ? "Required for the selected IDE." : "Optional IDE integration.");
		AppendKnownToolStatus(status, KnownTool::Git, true, "Minimum required version: " + std::string(kMinimumGitVersion));
		AppendKnownToolStatus(status, KnownTool::ClangFormat, false, "Required only for format operations.");

		status.VswherePath = ResolveVswherePath().value_or(std::filesystem::path());
		status.Items.push_back(MakeToolStatus(
		    "visualstudio",
		    "Visual Studio C++ tools",
		    visualStudioGenerator,
		    !status.VswherePath.empty(),
		    status.VswherePath,
		    !status.VswherePath.empty() ? "vswhere is available for C++ workload discovery: " + std::string(kVisualStudioCppComponent) : "vswhere was not found."));

		status.WindowsSdkVersion = FindWindowsSdkVersion().value_or(std::string());
		status.Items.push_back(MakeToolStatus(
		    "windowssdk",
		    "Windows SDK",
		    visualStudioGenerator,
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

		const QtKitDiscovery qtKit = DiscoverQtKit();
		status.QtRootPath = qtKit.QtRootPath;
		status.QtQmakePath = qtKit.QtQmakePath;
		status.Items.push_back(MakeToolStatus(
		    "qt-msvc",
		    "Qt 6 MSVC kit",
		    true,
		    qtKit.FoundMsvcKit,
		    qtKit.FoundMsvcKit ? qtKit.QtRootPath : (!qtKit.MingwCandidates.empty() ? qtKit.MingwCandidates.front() : std::filesystem::path()),
		    BuildQtStatusDetail(qtKit)));

#if SPARKLE_ENABLE_SHADER_COMPILER
		const ShaderCompilerSdkStatus shaderCompilerSdk = DetectShaderCompilerSdk();
		status.ShaderCompilerSdkRoot = shaderCompilerSdk.Root;
		status.ConfigurePrerequisitesAvailable = shaderCompilerSdk.Available;
		status.Items.push_back(MakeToolStatus(
		    "shader-compiler-sdk",
		    "Shader compiler SDK (DXC + Slang)",
		    false,
		    shaderCompilerSdk.Available,
		    shaderCompilerSdk.Root,
		    shaderCompilerSdk.Detail));
#else
		status.ConfigurePrerequisitesAvailable = true;
#endif

		const bool vulkanSdkRequired = featureSettings.NvidiaStreamlineEnabled;
		const VulkanSdkStatus vulkanSdk = DetectVulkanSdk();
		status.VulkanSdkRoot = vulkanSdk.Root;
		status.Items.push_back(MakeToolStatus(
		    "vulkan-sdk",
		    "Vulkan SDK",
		    false,
		    vulkanSdk.Available,
		    vulkanSdk.Root,
		    vulkanSdkRequired ?
		        (vulkanSdk.Available ? "Required for enabled NVIDIA Streamline and Vulkan-backed renderer integrations. " + vulkanSdk.Detail : vulkanSdk.Detail) :
		        (vulkanSdk.Available ? "Optional Vulkan SDK root: " + vulkanSdk.Root.string() : "Optional unless Vulkan-backed integrations are enabled.")));
		if (vulkanSdkRequired)
		{
			status.ConfigurePrerequisitesAvailable = status.ConfigurePrerequisitesAvailable && vulkanSdk.Available;
		}

		status.RequiredToolsAvailable = AreRequiredToolsAvailable(status.Items);
		return status;
	}
}

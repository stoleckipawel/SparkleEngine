#include "SparkleLauncher/BuildWorkspaceOperations.h"

#include "CMakeGeneratorModel.h"
#include "HostToolInstaller.h"
#include "QtToolchainDiscovery.h"
#include "ShaderCompilerSdkDiscovery.h"
#include "VisualStudioToolchainDiscovery.h"
#include "VulkanSdkDiscovery.h"
#include "Core/Public/Environment/EnvironmentVariables.h"
#include "Core/Public/Strings/StringUtils.h"
#include "SparkleLauncher/ToolResolver.h"

#include <algorithm>
#include <optional>
#include <utility>

namespace SparkleLauncher
{
	static constexpr std::string_view kVisualStudioCppComponent = "Microsoft.VisualStudio.Component.VC.Tools.x86.x64";
	static constexpr std::string_view kMinimumCMakeVersion = "3.20.0";
	static constexpr std::string_view kMinimumGitVersion = "2.25.0";

	static std::string ResolveGenerator()
	{
		std::string overrideGenerator;
		if (Environment::TryGetVariable("SPARKLE_CMAKE_GENERATOR", overrideGenerator))
		{
			return overrideGenerator;
		}
		return ResolveVisualStudioGenerator();
	}

	static std::string ResolvePlatform()
	{
		std::string architecture;
		return Environment::TryGetVariable("SPARKLE_CMAKE_ARCH", architecture) ? architecture : "x64";
	}

	static std::string ResolveToolset(WorkspaceCompiler compiler)
	{
		return compiler == WorkspaceCompiler::ClangCl ? "ClangCL" : std::string();
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
			case KnownTool::Ninja:
				status.NinjaPath = resolvedTool.Path;
				break;
			case KnownTool::Rider:
				status.RiderPath = resolvedTool.Path;
				break;
			case KnownTool::Git:
				status.GitPath = resolvedTool.Path;
				break;
		}
	}

	static bool AreRequiredToolsAvailable(const std::vector<ToolchainItemStatus>& items)
	{
		return std::none_of(
		    items.begin(),
		    items.end(),
		    [](const ToolchainItemStatus& item) { return item.Required && item.State != ToolchainItemState::Found; });
	}

	BuildToolchainStatus DetectBuildToolchain(
	    const std::filesystem::path& repositoryRoot,
	    WorkspaceIde preferredIde,
	    WorkspaceCompiler compiler)
	{
		(void) repositoryRoot;
		BuildToolchainStatus status;
		const WorkspaceFeatureSettings featureSettings = GetLauncherWorkspaceFeatureSettings();
		status.Generator = ResolveGenerator();
		status.Platform = ResolvePlatform();
		status.Compiler = compiler;
		status.Toolset = ResolveToolset(compiler);
		const bool visualStudioGenerator = GetCMakeGeneratorFamily(status.Generator) == CMakeGeneratorFamily::VisualStudio;
		const bool ninjaGenerator = CMakeGeneratorUsesNinjaMakeProgram(status.Generator);

		AppendKnownToolStatus(status, KnownTool::CMake, true, "Minimum required version: " + std::string(kMinimumCMakeVersion));
		AppendKnownToolStatus(
		    status,
		    KnownTool::MSBuild,
		    visualStudioGenerator,
		    visualStudioGenerator ? "Required by the selected CMake generator." : "Optional for non-Visual Studio generators.");
		AppendKnownToolStatus(
		    status,
		    KnownTool::Ninja,
		    ninjaGenerator,
		    ninjaGenerator ? "Required by the selected CMake generator." : "Optional for Ninja generators.");
		AppendKnownToolStatus(
		    status,
		    KnownTool::Rider,
		    false,
		    preferredIde == WorkspaceIde::Rider ? "Selected IDE integration. Required only when opening the IDE."
		                                        : "Optional IDE integration.");
		AppendKnownToolStatus(status, KnownTool::Git, true, "Minimum required version: " + std::string(kMinimumGitVersion));

		const VisualStudioToolchainDiscovery visualStudio = DiscoverVisualStudioToolchain();
		status.VswherePath = visualStudio.DiscoveryPath;
		status.VisualStudioPath = visualStudio.InstallationPath;
		status.VisualStudioIdePath = visualStudio.IdePath;
		status.VisualStudioInstallerPath = visualStudio.InstallerPath;
		status.ClangClPath = visualStudio.ClangClPath;
		status.WindowsSdkVersion = visualStudio.WindowsSdkVersion;
		status.Items.push_back(MakeToolStatus(
		    "visualstudio",
		    "Visual Studio C++ tools",
		    visualStudioGenerator,
		    !status.VisualStudioPath.empty(),
		    status.VisualStudioPath,
		    !status.VisualStudioPath.empty()
		        ? "Visual Studio C++ tools are available for generator/workload discovery: " + std::string(kVisualStudioCppComponent)
		        : "Visual Studio C++ tools were not found."));
		status.Items.push_back(MakeToolStatus(
		    "visualstudio-ide",
		    "Visual Studio IDE",
		    false,
		    !status.VisualStudioIdePath.empty(),
		    status.VisualStudioIdePath,
		    !status.VisualStudioIdePath.empty() ? "Visual Studio IDE is available."
		                                        : "Visual Studio C++ build tools are installed without the Visual Studio IDE."));
		status.Items.push_back(MakeToolStatus(
		    "windowssdk",
		    "Windows SDK",
		    visualStudioGenerator,
		    !status.WindowsSdkVersion.empty(),
		    {},
		    !status.WindowsSdkVersion.empty() ? "Latest SDK: " + status.WindowsSdkVersion
		                                      : "Windows Kits 10 Include directory was not found."));

		const bool clangClRequired = compiler == WorkspaceCompiler::ClangCl;
		if (status.ClangClPath.empty())
		{
			status.ClangClPath = FindExecutableOnPath("clang-cl.exe").value_or(std::filesystem::path());
		}
		ToolchainItemStatus clangCl = MakeToolStatus(
		    "clangcl",
		    "clang-cl",
		    clangClRequired,
		    !status.ClangClPath.empty(),
		    status.ClangClPath,
		    clangClRequired ? "Selected compiler. The launcher configures CMake with the Visual Studio ClangCL toolset."
		                    : (!status.ClangClPath.empty() ? "Available as a launcher compiler choice." : "Not installed."));
		clangCl.Compiler = WorkspaceCompiler::ClangCl;
		status.Items.push_back(std::move(clangCl));

		const QtToolchainDiscovery qt = DiscoverQtToolchain();
		status.QtRootPath = qt.QtRootPath;
		status.QtQmakePath = qt.QtQmakePath;
		status.Items.push_back(MakeToolStatus(
		    "qt-msvc",
		    "Qt 6 MSVC kit",
		    true,
		    qt.FoundMsvcKit,
		    qt.FoundMsvcKit ? qt.QtRootPath : (!qt.MingwCandidates.empty() ? qt.MingwCandidates.front() : std::filesystem::path()),
		    BuildQtToolchainStatusDetail(qt)));

#if SPARKLE_ENABLE_SHADER_COMPILER
		const ShaderCompilerSdkStatus shaderCompilerSdk = DetectShaderCompilerSdk();
		status.ShaderCompilerSdkRoot = shaderCompilerSdk.Root;
		status.ConfigurePrerequisitesAvailable = shaderCompilerSdk.Available;
		status.Items.push_back(MakeToolStatus(
		    "shader-compiler-sdk",
		    "Shader compiler SDK (DXC + Slang bundle)",
		    true,
		    shaderCompilerSdk.Available,
		    shaderCompilerSdk.Root,
		    shaderCompilerSdk.Detail));
#else
		status.ConfigurePrerequisitesAvailable = true;
#endif

		const bool vulkanSdkRequired = featureSettings.NvidiaStreamlineEnabled;
		const VulkanSdkStatus vulkanSdk = DetectVulkanSdk();
		status.VulkanSdkRoot = vulkanSdk.Root;
		std::string vulkanDetail;
		if (vulkanSdkRequired)
		{
			vulkanDetail = vulkanSdk.Available
			    ? "Required for enabled NVIDIA Streamline and Vulkan-backed renderer integrations. " + vulkanSdk.Detail
			    : vulkanSdk.Detail;
		}
		else
		{
			vulkanDetail = vulkanSdk.Available ? "Optional Vulkan SDK root: " + vulkanSdk.Root.string()
			                                   : "Optional unless Vulkan-backed integrations are enabled.";
		}
		status.Items.push_back(
		    MakeToolStatus("vulkan-sdk", "Vulkan SDK", vulkanSdkRequired, vulkanSdk.Available, vulkanSdk.Root, std::move(vulkanDetail)));
		if (vulkanSdkRequired)
		{
			status.ConfigurePrerequisitesAvailable = status.ConfigurePrerequisitesAvailable && vulkanSdk.Available;
		}

		for (ToolchainItemStatus& item : status.Items)
		{
			item.CanInstall = CanInstallHostTool(item.Id, status);
			if (item.Id == "clangcl" && item.State != ToolchainItemState::Found)
			{
				item.Detail = item.CanInstall ? "Not installed. The launcher can add the Visual Studio clang-cl component."
				                              : "Not installed and no supported launcher installer is available on this machine.";
			}
		}

		status.RequiredToolsAvailable = AreRequiredToolsAvailable(status.Items);
		return status;
	}
}

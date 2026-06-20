#include "CMakeWorkflowProcessRequests.h"

#include "CMakeGeneratorModel.h"
#include "SparkleLauncher/LauncherPaths.h"

namespace SparkleLauncher
{
	static std::vector<std::string> GetDefaultBuildToolArguments()
	{
		return {"/nologo", "/v:minimal", "/m:1", "/p:UseMultiToolTask=false", "/p:TrackFileAccess=false", "/nodeReuse:false"};
	}

	static std::string ToCMakeBool(bool value)
	{
		return value ? "ON" : "OFF";
	}

	ProcessRequest MakeCMakeConfigureRequest(
	    const std::filesystem::path& repositoryRoot,
	    const BuildToolchainStatus& toolchain,
	    std::string_view operationId,
	    std::string_view logFileName)
	{
		ProcessRequest process;
		process.ExecutablePath = toolchain.CMakePath;
		process.WorkingDirectory = GetBuildDirectory(repositoryRoot);
		process.LogPath = GetLauncherOperationLogPath(repositoryRoot, operationId, logFileName);
		process.Arguments = {"-G", toolchain.Generator};
		if (CMakeGeneratorUsesPlatformArgument(toolchain.Generator))
		{
			process.Arguments.push_back("-A");
			process.Arguments.push_back(toolchain.Platform);
		}
		if (!toolchain.Toolset.empty() && CMakeGeneratorUsesToolsetArgument(toolchain.Generator))
		{
			process.Arguments.push_back("-T");
			process.Arguments.push_back(toolchain.Toolset);
		}
		if (!toolchain.NinjaPath.empty() && CMakeGeneratorUsesNinjaMakeProgram(toolchain.Generator))
		{
			process.Arguments.push_back("-DCMAKE_MAKE_PROGRAM=" + toolchain.NinjaPath.generic_string());
		}
		if (!toolchain.QtRootPath.empty())
		{
			process.Arguments.push_back("-DCMAKE_PREFIX_PATH=" + toolchain.QtRootPath.generic_string());
		}
		if (!toolchain.GitPath.empty())
		{
			process.Arguments.push_back("-DGIT_EXECUTABLE=" + toolchain.GitPath.generic_string());
			process.Arguments.push_back("-DSPARKLE_GIT_EXE=" + toolchain.GitPath.generic_string());
		}
		if (!toolchain.VulkanSdkRoot.empty())
		{
			process.Arguments.push_back("-DVulkan_INCLUDE_DIR=" + (toolchain.VulkanSdkRoot / "Include").generic_string());
			process.Arguments.push_back("-DVulkan_LIBRARY=" + (toolchain.VulkanSdkRoot / "Lib" / "vulkan-1.lib").generic_string());
			process.Environment.push_back({"VULKAN_SDK", toolchain.VulkanSdkRoot.generic_string()});
			process.Environment.push_back({"VK_SDK_PATH", toolchain.VulkanSdkRoot.generic_string()});
		}
		const WorkspaceFeatureSettings featureSettings = GetLauncherWorkspaceFeatureSettings();
		process.Arguments.push_back("-DSPARKLE_ENABLE_CONTENT_PIPELINE=" + ToCMakeBool(featureSettings.ContentPipelineEnabled));
		process.Arguments.push_back("-DSPARKLE_ENABLE_SHADER_COMPILER=" + ToCMakeBool(featureSettings.ShaderCompilerEnabled));
		process.Arguments.push_back("-DSPARKLE_ENABLE_KTX_SUPPORT=" + ToCMakeBool(featureSettings.KtxSupportEnabled));
		process.Arguments.push_back("-DSPARKLE_ENABLE_NVIDIA_STREAMLINE=" + ToCMakeBool(featureSettings.NvidiaStreamlineEnabled));
		process.Arguments.push_back("-Wno-dev");
		process.Arguments.push_back(repositoryRoot.string());
		return process;
	}

	ProcessRequest MakeCMakeBuildRequest(
	    const std::filesystem::path& repositoryRoot,
	    const BuildToolchainStatus& toolchain,
	    std::string_view operationId,
	    std::string_view profileName,
	    const std::vector<std::string>& targets,
	    std::string_view logFileName)
	{
		ProcessRequest process;
		process.ExecutablePath = toolchain.CMakePath;
		process.WorkingDirectory = repositoryRoot;
		process.LogPath = GetLauncherOperationLogPath(repositoryRoot, operationId, logFileName);
		process.Arguments = {"--build", GetBuildDirectory(repositoryRoot).string(), "--config", std::string(profileName), "--target"};
		process.Arguments.insert(process.Arguments.end(), targets.begin(), targets.end());
		if (CMakeGeneratorUsesMsBuildArguments(toolchain.Generator))
		{
			process.Arguments.push_back("--");
			const std::vector<std::string> buildToolArguments = GetDefaultBuildToolArguments();
			process.Arguments.insert(process.Arguments.end(), buildToolArguments.begin(), buildToolArguments.end());
		}
		return process;
	}
}

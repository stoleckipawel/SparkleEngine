#include "CMakeWorkflowProcessRequests.h"

#include "SparkleLauncher/LauncherPaths.h"

namespace SparkleLauncher
{
	static std::vector<std::string> GetDefaultBuildToolArguments()
	{
		return {"/nologo", "/v:minimal", "/m:1", "/p:UseMultiToolTask=false", "/p:TrackFileAccess=false", "/nodeReuse:false"};
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
		process.Arguments = {"-G", toolchain.Generator, "-A", toolchain.Platform};
		if (!toolchain.Toolset.empty())
		{
			process.Arguments.push_back("-T");
			process.Arguments.push_back(toolchain.Toolset);
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
		process.Arguments.push_back("--");
		const std::vector<std::string> buildToolArguments = GetDefaultBuildToolArguments();
		process.Arguments.insert(process.Arguments.end(), buildToolArguments.begin(), buildToolArguments.end());
		return process;
	}
}

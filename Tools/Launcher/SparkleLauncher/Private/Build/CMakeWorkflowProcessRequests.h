#pragma once

#include "SparkleLauncher/BuildWorkspaceOperations.h"
#include "SparkleLauncher/ProcessRunner.h"

#include <filesystem>
#include <string>
#include <string_view>
#include <vector>

namespace SparkleLauncher
{
	ProcessRequest MakeCMakeConfigureRequest(
	    const std::filesystem::path& repositoryRoot,
	    const BuildToolchainStatus& toolchain,
	    std::string_view operationId,
	    std::string_view logFileName);

	ProcessRequest MakeCMakeDependencySyncRequest(
	    const std::filesystem::path& repositoryRoot,
	    const BuildToolchainStatus& toolchain,
	    std::string_view operationId,
	    std::string_view sourceDependencyId,
	    std::string_view logFileName);

	ProcessRequest MakeCMakeBuildRequest(
	    const std::filesystem::path& repositoryRoot,
	    const BuildToolchainStatus& toolchain,
	    std::string_view operationId,
	    std::string_view profileName,
	    const std::vector<std::string>& targets,
	    std::string_view logFileName);
}

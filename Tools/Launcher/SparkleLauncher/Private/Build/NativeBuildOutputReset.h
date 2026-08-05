#pragma once

#include "SparkleLauncher/BuildWorkspaceOperations.h"

#include <filesystem>
#include <string>

namespace SparkleLauncher
{
	bool RequiresNativeBuildOutputReset(BuildFilesFreshnessState state);
	bool ResetNativeBuildOutputs(
	    const std::filesystem::path& repositoryRoot,
	    const std::filesystem::path& buildDirectory,
	    std::string& errorMessage);
}

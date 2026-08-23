#pragma once

#include "Backend/ShaderTarget.h"

#include <cstdint>
#include <filesystem>
#include <string>
#include <vector>

struct ShaderPackageCookSettings final
{
	std::vector<ShaderTarget> targets = {ShaderTarget::DxilSm66, ShaderTarget::SpirV16};
	std::string backendName = "auto";
	std::string packageId;
	std::string shaderId;
	std::vector<std::string> changedVirtualPaths;
	std::filesystem::path cancellationSignalPath;
	std::filesystem::path debugArtifactDirectory;
	std::vector<std::string> analysisPasses;
	bool enableDebugInfo = false;
	bool enableOptimizations = true;
	bool treatWarningsAsErrors = true;
	bool stripDebugInfo = true;
	std::uint32_t maximumParallelCompiles = 4;
};

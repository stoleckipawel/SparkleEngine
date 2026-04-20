#pragma once

#include "RHI/Public/Shaders/CookedShaderPackage.h"

#include <cstdint>
#include <string>
#include <vector>

struct CookedStageBuild final
{
	ShaderStage stage = ShaderStage::Count;
	std::string sourcePath;
	std::string entryPoint;
	std::string debugArtifact;
	std::vector<std::uint8_t> bytecode;
	std::uint64_t bytecodeHash = 0;
};

#pragma once

#include "Cooking/CookedShaderPackageOutput.h"

#include <cstdint>
#include <filesystem>
#include <vector>

struct ShaderPackageCookResult final
{
	std::filesystem::path outputDirectory;
	std::vector<CookedShaderPackageOutput> packages;
	std::size_t selectedShaderCount = 0;
	std::size_t compileJobCount = 0;
};

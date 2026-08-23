#pragma once

#include "Cooking/CookedShaderPackageOutput.h"

#include <cstdint>
#include <filesystem>
#include <vector>

struct ShaderPackageCookResult final
{
	std::filesystem::path outputDirectory;
	std::vector<CookedShaderPackageOutput> packages;
};

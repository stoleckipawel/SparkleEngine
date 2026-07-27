#pragma once

#include "Cooking/CookedShaderPackageOutput.h"

#include <cstdint>
#include <filesystem>
#include <string>
#include <vector>

struct ShaderPackageCookResult final
{
	std::filesystem::path cacheDirectory;
	std::vector<CookedShaderPackageOutput> packages;
	std::string errorMessage;

	bool Succeeded() const noexcept { return errorMessage.empty(); }
};

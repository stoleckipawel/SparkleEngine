#pragma once

#include "Cooking/CookedShaderPackageOutput.h"

#include <cstdint>
#include <filesystem>
#include <string>
#include <vector>

struct ShaderPackageCookResult final
{
	std::filesystem::path registryPath;
	std::filesystem::path cacheDirectory;
	std::filesystem::path recookSignalPath;
	std::uint64_t recookSignalRegistryHash = 0;
	std::vector<CookedShaderPackageOutput> packages;
	std::string errorMessage;
	std::size_t backendInvocationCount = 0;
	std::size_t cacheHitCount = 0;
	std::size_t cacheMissCount = 0;

	bool Succeeded() const noexcept { return errorMessage.empty(); }
};

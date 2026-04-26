#pragma once

#include "Backend/ShaderTarget.h"
#include "Cooking/CookedShaderPackageOutput.h"
#include "Cooking/ShaderCookTypes.h"

#include <cstdint>
#include <filesystem>
#include <string>
#include <vector>

struct ShaderPackageCookSettings final
{
	bool useCache = true;
	ShaderTarget target = kDefaultShaderTarget;
	std::string backendName = "auto";
	std::filesystem::path singleShaderPath;
	std::filesystem::path cacheDirectory;
	std::filesystem::path debugArtifactDirectory;
	std::vector<std::string> analysisPasses;
	bool forceParameterStructMismatchForValidation = false;
};

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

class ShaderPackageCooker final
{
  public:
	ShaderPackageCookResult CookAll(const ShaderPackageCookSettings& settings = {}) const;
};

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
	std::vector<ShaderTarget> targets = {ShaderTarget::DxilSm66, ShaderTarget::SpirV16};
	std::string backendName = "auto";
	std::string packageId;
	std::string shaderId;
	std::filesystem::path cacheDirectory;
	std::filesystem::path debugArtifactDirectory;
	std::vector<std::string> analysisPasses;
	bool forceParameterStructMismatchForValidation = false;
	bool forceMissingIncludeForValidation = false;
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

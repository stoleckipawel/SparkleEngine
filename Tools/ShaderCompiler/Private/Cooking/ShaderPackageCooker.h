#pragma once

#include "Backend/ShaderTarget.h"
#include "Cooking/CookedShaderPackageOutput.h"

#include <filesystem>
#include <string>
#include <vector>

struct ShaderPackageCookSettings final
{
	bool useCache = true;
	ShaderTarget target = kDefaultShaderTarget;
	std::filesystem::path cacheDirectory;
};

struct ShaderPackageCookResult final
{
	std::filesystem::path registryPath;
	std::filesystem::path cacheDirectory;
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

  private:
	static std::filesystem::path ResolveCacheDirectory(const ShaderPackageCookSettings& settings);
};

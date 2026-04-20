#pragma once

#include "Cooking/CookedShaderPackageOutput.h"

#include <filesystem>
#include <string>
#include <vector>

struct ShaderPackageCookResult final
{
	std::filesystem::path registryPath;
	std::vector<CookedShaderPackageOutput> packages;
	std::string errorMessage;

	bool Succeeded() const noexcept { return errorMessage.empty(); }
};

class ShaderPackageCooker final
{
  public:
	ShaderPackageCookResult CookAll() const;
};
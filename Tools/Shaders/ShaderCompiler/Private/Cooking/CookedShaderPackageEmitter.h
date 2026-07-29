#pragma once

#include "Cooking/CookedShaderPackageOutput.h"
#include "Cooking/ShaderCookContext.h"

#include <filesystem>
#include <vector>

class CookedShaderPackageEmitter final
{
  public:
	CookedShaderPackageEmitter() = delete;

	static std::vector<CookedShaderPackageOutput> Emit(
	    const ShaderCookPipelinePlan& plan,
	    const std::filesystem::path& cacheDirectory);
};

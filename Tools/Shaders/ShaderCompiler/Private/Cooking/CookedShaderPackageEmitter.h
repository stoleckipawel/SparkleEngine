#pragma once

#include "Cooking/ShaderCookContext.h"

#include <filesystem>
#include <string>

struct ShaderPackageCookResult;

class CookedShaderPackageEmitter final
{
  public:
	CookedShaderPackageEmitter() = delete;

	static bool Emit(
	    const ShaderCookPipelinePlan& plan,
	    const std::filesystem::path& cacheDirectory,
	    ShaderPackageCookResult& result,
	    std::string& outErrorMessage);
};
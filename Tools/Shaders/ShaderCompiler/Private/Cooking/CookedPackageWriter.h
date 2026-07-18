#pragma once

#include "Cooking/CookedShaderPackageOutput.h"
#include "Cooking/CookedStageBuild.h"
#include "Cooking/ShaderCookTypes.h"

#include <span>
#include <filesystem>
#include <string>

class CookedPackageWriter final
{
  public:
	static bool Write(
	    const ShaderCookPackageDesc& package,
	    std::span<const CookedStageBuild> compiledStages,
	    const std::filesystem::path& storagePath,
	    const std::filesystem::path& publishedPath,
	    CookedShaderPackageOutput& outPackageOutput,
	    std::string& outErrorMessage);
};

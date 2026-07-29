#pragma once

#include "Cooking/CookedShaderPackageOutput.h"
#include "Cooking/CookedStageBuild.h"
#include "Cooking/ShaderCookTypes.h"

#include <filesystem>
#include <span>

class CookedPackageWriter final
{
  public:
	static CookedShaderPackageOutput Write(
	    const ShaderCookPackageDesc& package,
	    std::span<const CookedStageBuild> compiledStages,
	    const std::filesystem::path& storagePath,
	    const std::filesystem::path& publishedPath);
};

#pragma once

#include "Cooking/CookedShaderPackageOutput.h"
#include "Cooking/CookedStageBuild.h"
#include "Cooking/ShaderCookTypes.h"

#include <span>
#include <string>

class CookedPackageWriter final
{
  public:
	static bool Write(
	    const ShaderCookPackageDesc& package,
	    std::span<const CookedStageBuild> compiledStages,
	    CookedShaderPackageOutput& outPackageOutput,
	    std::string& outErrorMessage);
};

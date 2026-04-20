#pragma once

#include "Cooking/CookedShaderPackageOutput.h"
#include "Cooking/CookedStageBuild.h"
#include "Manifest/ShaderCookManifestTypes.h"
#include "RHI/Public/Shaders/ShaderPackageLayoutCatalog.h"

#include <span>
#include <string>

class CookedPackageWriter final
{
  public:
	static bool Write(
	    const ShaderCookPackageDesc& package,
	    const PassParameterLayout& bindingLayout,
	    std::span<const CookedStageBuild> compiledStages,
	    CookedShaderPackageOutput& outPackageOutput,
	    std::string& outErrorMessage);
};

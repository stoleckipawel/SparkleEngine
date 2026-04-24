#pragma once

#include "Cooking/CookedStageBuild.h"
#include "Manifest/ShaderCookManifestTypes.h"
#include "ShaderCompileOptions.h"

#include <string>

class IShaderBackend;

class StageCompiler final
{
  public:
	static bool Compile(
	    IShaderBackend& backend,
	    const ShaderCookStageDesc& stage,
	    const ShaderCompileOptions& options,
	    CookedStageBuild& outCompiledStage,
	    std::string& outErrorMessage);
};

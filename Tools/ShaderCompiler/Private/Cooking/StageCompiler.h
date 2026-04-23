#pragma once

#include "Cooking/CookedStageBuild.h"
#include "Manifest/ShaderCookManifestTypes.h"
#include "RHI/Public/Shaders/ShaderCompileOptions.h"

#include <string>

class StageCompiler final
{
  public:
	static bool Compile(
	    const ShaderCookStageDesc& stage,
	    const ShaderCompileOptions& options,
	    CookedStageBuild& outCompiledStage,
	    std::string& outErrorMessage);
};

#pragma once

#include "Cooking/CookedStageBuild.h"
#include "Manifest/ShaderCookManifestTypes.h"

#include <string>

class StageCompiler final
{
  public:
	static bool Compile(
	    const ShaderCookStageDesc& stage,
	    CookedStageBuild& outCompiledStage,
	    std::string& outErrorMessage);
};

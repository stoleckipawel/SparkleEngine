#pragma once

#include "Cooking/CookedStageBuild.h"
#include "Cooking/ShaderCookTypes.h"
#include "ShaderDebugArtifactSet.h"
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
	    ShaderDebugArtifactSet* outDebugArtifacts,
	    std::string& outErrorMessage);
};

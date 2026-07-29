#pragma once

#include "Cooking/CookedStageBuild.h"
#include "Cooking/ShaderCookTypes.h"
#include "ShaderDebugArtifactSet.h"
#include "ShaderCompileOptions.h"

class IShaderBackend;

class StageCompiler final
{
  public:
	static CookedStageBuild Compile(
	    IShaderBackend& backend,
	    const ShaderCookStageDesc& stage,
	    const ShaderCompileOptions& options,
	    ShaderDebugArtifactSet* outDebugArtifacts);
};

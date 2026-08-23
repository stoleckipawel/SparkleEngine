#pragma once

#include "Compiler/ShaderCompileRequest.h"
#include "Cooking/CookedStageBuild.h"
#include "ShaderDebugArtifactSet.h"

class IShaderBackend;

class StageCompiler final
{
public:
	static CookedStageBuild Compile(
	    IShaderBackend& backend,
	    const ShaderCompileRequest& request,
	    ShaderDebugArtifactSet* outDebugArtifacts);
};

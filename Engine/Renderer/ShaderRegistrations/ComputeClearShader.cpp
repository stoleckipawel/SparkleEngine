#include "PCH.h"

#include "RendererShaderPackages.h"
#include "Shaders/Authoring/GlobalShader.h"

class ComputeClearCS final : public TGlobalShader<ComputeClearCS>
{
  public:
	BEGIN_SHADER_PARAMETER_STRUCT(FParameters, )
	SHADER_PARAMETER_UAV_NAMED(RWTexture2D, Output, OutputTexture)
	END_SHADER_PARAMETER_STRUCT()
};

IMPLEMENT_GLOBAL_SHADER_IN_PACKAGE(
    ComputeClearCS,
    RendererShaderPackages::ComputeClear,
    "Passes/Compute/ComputeClear.hlsl",
    "main",
    Compute);

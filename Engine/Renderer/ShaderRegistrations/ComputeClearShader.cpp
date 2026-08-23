#include "PCH.h"

#include "RendererShaderPackages.h"
#include "Shaders/Authoring/GlobalShader.h"
#include "Renderer/Public/ShaderParameters/ShaderParameterStruct.h"

class ComputeClearCS final : public GlobalShader<ComputeClearCS>
{
  public:
	BEGIN_SHADER_PARAMETER_STRUCT(Parameters, )
	SHADER_PARAMETER_TEXTURE_UAV_NAMED(RWTexture2D, Output, OutputTexture)
	END_SHADER_PARAMETER_STRUCT()
};

IMPLEMENT_GLOBAL_SHADER_IN_PACKAGE(
    ComputeClearCS,
    RendererShaderPackages::ComputeClear,
    "/Engine/Passes/Compute/ComputeClear.hlsl",
    "main",
    Compute);

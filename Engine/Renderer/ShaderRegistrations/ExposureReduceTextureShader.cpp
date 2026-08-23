#include "PCH.h"

#include "RendererShaderPackages.h"
#include "Shaders/Authoring/GlobalShader.h"
#include "Renderer/Public/ShaderParameters/ShaderParameterStruct.h"

class ExposureReduceTextureCS final : public GlobalShader<ExposureReduceTextureCS>
{
  public:
	BEGIN_SHADER_PARAMETER_STRUCT(Parameters, )
	SHADER_PARAMETER_TEXTURE_SRV(Texture2D, LuminanceMomentsInput)
	SHADER_PARAMETER_TEXTURE_UAV(RWTexture2D, LuminanceMomentsOutput)
	END_SHADER_PARAMETER_STRUCT()
};

IMPLEMENT_GLOBAL_SHADER_IN_PACKAGE(
    ExposureReduceTextureCS,
    RendererShaderPackages::ExposureReduceTexture,
    "/Engine/Passes/PostProcessing/ExposureReduceTexture.hlsl",
    "main",
    Compute);

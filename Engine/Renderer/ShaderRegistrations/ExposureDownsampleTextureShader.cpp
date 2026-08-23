#include "PCH.h"

#include "RendererShaderPackages.h"
#include "Shaders/Authoring/GlobalShader.h"
#include "Renderer/Public/ShaderParameters/ShaderParameterStruct.h"

class ExposureDownsampleTextureCS final : public GlobalShader<ExposureDownsampleTextureCS>
{
  public:
	BEGIN_SHADER_PARAMETER_STRUCT(Parameters, )
	SHADER_PARAMETER_TEXTURE_SRV(Texture2D, LuminanceMomentsInput)
	SHADER_PARAMETER_TEXTURE_UAV(RWTexture2D, LuminanceMomentsOutput)
	END_SHADER_PARAMETER_STRUCT()
};

IMPLEMENT_GLOBAL_SHADER_IN_PACKAGE(
    ExposureDownsampleTextureCS,
    RendererShaderPackages::ExposureDownsampleTexture,
    "/Engine/Passes/PostProcessing/ExposureDownsampleTexture.hlsl",
    "main",
    Compute);

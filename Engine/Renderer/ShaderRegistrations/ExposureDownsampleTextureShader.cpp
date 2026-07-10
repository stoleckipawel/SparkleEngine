#include "PCH.h"

#include "RendererShaderPackages.h"
#include "Shaders/Authoring/GlobalShader.h"

class ExposureDownsampleTextureCS final : public TGlobalShader<ExposureDownsampleTextureCS>
{
  public:
	BEGIN_SHADER_PARAMETER_STRUCT(FParameters, )
	SHADER_PARAMETER_TEXTURE(Texture2D, LuminanceMomentsInput)
	SHADER_PARAMETER_UAV(RWTexture2D, LuminanceMomentsOutput)
	END_SHADER_PARAMETER_STRUCT()
};

IMPLEMENT_GLOBAL_SHADER_IN_PACKAGE(
    ExposureDownsampleTextureCS,
    RendererShaderPackages::ExposureDownsampleTexture,
    "Passes/PostProcessing/ExposureDownsampleTexture.hlsl",
    "main",
    Compute);

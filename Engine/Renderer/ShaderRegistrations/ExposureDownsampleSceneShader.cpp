#include "PCH.h"

#include "RendererShaderPackages.h"
#include "Shaders/Authoring/GlobalShader.h"

class ExposureDownsampleSceneCS final : public TGlobalShader<ExposureDownsampleSceneCS>
{
  public:
	BEGIN_SHADER_PARAMETER_STRUCT(FParameters, )
	SHADER_PARAMETER_TEXTURE(Texture2D, SceneColor)
	SHADER_PARAMETER_UAV(RWTexture2D, LuminanceMomentsOutput)
	END_SHADER_PARAMETER_STRUCT()
};

IMPLEMENT_GLOBAL_SHADER_IN_PACKAGE(
    ExposureDownsampleSceneCS,
    RendererShaderPackages::ExposureDownsampleScene,
    "Passes/PostProcessing/ExposureDownsampleScene.hlsl",
    "main",
    Compute);

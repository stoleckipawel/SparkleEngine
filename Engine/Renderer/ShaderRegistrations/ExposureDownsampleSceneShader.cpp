#include "PCH.h"

#include "RendererShaderPackages.h"
#include "Shaders/Authoring/GlobalShader.h"
#include "Renderer/Public/ShaderParameters/ShaderParameterStruct.h"

class ExposureDownsampleSceneCS final : public GlobalShader<ExposureDownsampleSceneCS>
{
  public:
	BEGIN_SHADER_PARAMETER_STRUCT(Parameters, )
	SHADER_PARAMETER_TEXTURE_SRV(Texture2D, SceneColor)
	SHADER_PARAMETER_TEXTURE_UAV(RWTexture2D, LuminanceMomentsOutput)
	END_SHADER_PARAMETER_STRUCT()
};

IMPLEMENT_GLOBAL_SHADER_IN_PACKAGE(
    ExposureDownsampleSceneCS,
    RendererShaderPackages::ExposureDownsampleScene,
    "/Engine/Passes/PostProcessing/ExposureDownsampleScene.hlsl",
    "main",
    Compute);

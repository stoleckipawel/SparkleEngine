#include "PCH.h"

#include "RendererShaderPackages.h"
#include "Shaders/Authoring/GlobalShader.h"
#include "Renderer/Public/ShaderParameters/ShaderParameterStruct.h"

class ExposureReduceSceneCS final : public GlobalShader<ExposureReduceSceneCS>
{
  public:
	BEGIN_SHADER_PARAMETER_STRUCT(Parameters, )
	SHADER_PARAMETER_TEXTURE_SRV(Texture2D, SceneColor)
	SHADER_PARAMETER_TEXTURE_UAV(RWTexture2D, LuminanceMomentsOutput)
	END_SHADER_PARAMETER_STRUCT()
};

IMPLEMENT_GLOBAL_SHADER_IN_PACKAGE(
    ExposureReduceSceneCS,
    RendererShaderPackages::ExposureReduceScene,
    "/Engine/Passes/PostProcessing/ExposureReduceScene.hlsl",
    "main",
    Compute);

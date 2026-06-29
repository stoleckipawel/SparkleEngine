#include "PCH.h"

#include "RendererShaderPackages.h"
#include "Shaders/Authoring/GlobalShader.h"

class ExposureReduceSceneCS final : public TGlobalShader<ExposureReduceSceneCS>
{
  public:
	BEGIN_SHADER_PARAMETER_STRUCT(FParameters, )
	SHADER_PARAMETER_TEXTURE(Texture2D, SceneColor)
	SHADER_PARAMETER_UAV(RWTexture2D, LuminanceMomentsOutput)
	END_SHADER_PARAMETER_STRUCT()
};

IMPLEMENT_GLOBAL_SHADER_IN_PACKAGE(
    ExposureReduceSceneCS,
    RendererShaderPackages::ExposureReduceScene,
    "Passes/PostProcessing/ExposureReduce.hlsl",
    "ReduceSceneMain",
    Compute);

#include "PCH.h"

#include "RendererShaderPackages.h"
#include "Shaders/Authoring/GlobalShader.h"

class PresentSceneVS final : public TGlobalShader<PresentSceneVS>
{
  public:
	BEGIN_SHADER_PARAMETER_STRUCT(FParameters, )
	END_SHADER_PARAMETER_STRUCT()
};

IMPLEMENT_GLOBAL_SHADER_IN_PACKAGE(
    PresentSceneVS,
    RendererShaderPackages::PresentScene,
    "Passes/Presentation/PresentScene.hlsl",
    "VSMain",
    Vertex);

class PresentScenePS final : public TGlobalShader<PresentScenePS>
{
  public:
	BEGIN_SHADER_PARAMETER_STRUCT(FParameters, )
	SHADER_PARAMETER_TEXTURE(Texture2D, SceneColor)
	SHADER_PARAMETER_SHARED_SAMPLER(SamplerLinearClamp)
	END_SHADER_PARAMETER_STRUCT()
};

IMPLEMENT_GLOBAL_SHADER_IN_PACKAGE(
    PresentScenePS,
    RendererShaderPackages::PresentScene,
    "Passes/Presentation/PresentScene.hlsl",
    "PSMain",
    Pixel);

#include "PCH.h"

#include "RendererShaderPackages.h"
#include "Shaders/Authoring/GlobalShader.h"

#include "ShaderData/RenderConstantBufferData.h"

class SceneDepthCS final : public TGlobalShader<SceneDepthCS>
{
  public:
	BEGIN_SHADER_PARAMETER_STRUCT(FParameters, )
	SHADER_PARAMETER_TEXTURE(Texture2D, GBufferDeviceZ)
	SHADER_PARAMETER_UAV(RWTexture2D, SceneDepth)
	SHADER_PARAMETER_CBUFFER_NAMED(PerView, PerViewConstantBufferData, PerViewConstantBufferData)
	END_SHADER_PARAMETER_STRUCT()
};

IMPLEMENT_GLOBAL_SHADER_IN_PACKAGE(
    SceneDepthCS,
    RendererShaderPackages::SceneDepth,
    "Passes/Deferred/SceneDepth.hlsl",
    "main",
    Compute);

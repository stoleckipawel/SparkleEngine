#include "PCH.h"

#include "RendererShaderPackages.h"
#include "Shaders/Authoring/GlobalShader.h"

#include "ShaderData/RenderConstantBufferData.h"

class SkyCS final : public TGlobalShader<SkyCS>
{
  public:
	BEGIN_SHADER_PARAMETER_STRUCT(FParameters, )
	SHADER_PARAMETER_UAV_NAMED(RWTexture2D, SceneColor, SceneColorTexture)
	SHADER_PARAMETER_CBUFFER_NAMED(PerFrame, PerFrameConstantBufferData, PerFrameConstantBufferData)
	SHADER_PARAMETER_CBUFFER_NAMED(PerView, PerViewConstantBufferData, PerViewConstantBufferData)
	SHADER_PARAMETER_CBUFFER_NAMED(PerTemporal, PerTemporalConstantBufferData, PerTemporalConstantBufferData)
	SHADER_PARAMETER_TEXTURE(Texture2D, SceneDepth)
	SHADER_PARAMETER_TEXTURE(Texture2D, SkyTexture)
	SHADER_PARAMETER_SHARED_SAMPLER(SamplerLinearClamp)
	END_SHADER_PARAMETER_STRUCT()
};

IMPLEMENT_GLOBAL_SHADER_IN_PACKAGE(
    SkyCS,
    RendererShaderPackages::Sky,
    "Passes/Deferred/Sky.hlsl",
    "main",
    Compute);

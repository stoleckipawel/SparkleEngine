#include "PCH.h"

#include "RendererShaderPackages.h"
#include "Shaders/Authoring/GlobalShader.h"

class IndirectLightingCS final : public TGlobalShader<IndirectLightingCS>
{
  public:
	BEGIN_SHADER_PARAMETER_STRUCT(FParameters, )
	SHADER_PARAMETER_UAV_NAMED(RWTexture2D, IndirectDiffuse, IndirectDiffuseTexture)
	SHADER_PARAMETER_TEXTURE(Texture2D, GBufferNormal)
	SHADER_PARAMETER_TEXTURE(Texture2D, GBufferDeviceZ)
	SHADER_PARAMETER_TEXTURE(Texture2D, SkyTexture)
	SHADER_PARAMETER_SHARED_SAMPLER(SamplerLinearNoMipClamp)
	END_SHADER_PARAMETER_STRUCT()
};

IMPLEMENT_GLOBAL_SHADER_IN_PACKAGE(
    IndirectLightingCS,
    RendererShaderPackages::IndirectLighting,
    "Passes/Deferred/IndirectLighting.hlsl",
    "main",
    Compute);

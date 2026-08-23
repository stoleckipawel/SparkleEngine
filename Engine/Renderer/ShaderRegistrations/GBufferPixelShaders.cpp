#include "PCH.h"

#include "RendererShaderPackages.h"
#include "Shaders/Authoring/GlobalShader.h"
#include "Renderer/Public/ShaderParameters/ShaderParameterStruct.h"

#include "ShaderData/ViewUniformData.h"
#include "ShaderData/ViewTemporalUniformData.h"
#include "ShaderData/PerObjectConstantBufferData.h"

class GBufferPS final : public GlobalShader<GBufferPS>
{
public:
	BEGIN_SHADER_PARAMETER_STRUCT(Parameters, )
	SHADER_PARAMETER_CBUFFER_NAMED(View, ViewUniformData, ViewUniformData)
	SHADER_PARAMETER_CBUFFER_NAMED(PerObjectPS, PerObjectPSConstantBufferData, PerObjectPSConstantBufferData)
	SHADER_PARAMETER_CBUFFER_NAMED(ViewTemporal, ViewTemporalUniformData, ViewTemporalUniformData)
	SHADER_PARAMETER_TEXTURE_SRV(Texture2D, TextureBaseColor)
	SHADER_PARAMETER_TEXTURE_SRV(Texture2D, TextureNormal)
	SHADER_PARAMETER_TEXTURE_SRV(Texture2D, TextureRoughness)
	SHADER_PARAMETER_TEXTURE_SRV(Texture2D, TextureMetallic)
	SHADER_PARAMETER_TEXTURE_SRV(Texture2D, TextureOcclusion)
	SHADER_PARAMETER_TEXTURE_SRV(Texture2D, TextureEmissive)
	SHADER_PARAMETER_TEXTURE_SRV(Texture2D, TextureSubsurfaceColor)
	SHADER_PARAMETER_TEXTURE_SRV(Texture2D, TextureSubsurfaceStrength)
	SHADER_PARAMETER_SHARED_SAMPLER(SamplerAniso16xWrap)
	END_SHADER_PARAMETER_STRUCT()
};

IMPLEMENT_GLOBAL_SHADER_IN_PACKAGE(GBufferPS, RendererShaderPackages::GBuffer, "/Engine/Passes/GBuffer/GBufferPS.hlsl", "main", Pixel);

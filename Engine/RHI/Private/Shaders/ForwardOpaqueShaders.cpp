#include "PCH.h"

#include "Shaders/Authoring/GlobalShader.h"

#include "Resources/RenderConstantBufferData.h"

#include <string_view>

void RegisterForwardOpaqueShaders() noexcept
{
}

class ForwardOpaqueVS final : public TGlobalShader<ForwardOpaqueVS>
{
  public:
	static constexpr std::string_view kShaderName = "ForwardOpaqueVS";
	static constexpr std::string_view kShaderPackageName = "ForwardOpaque";
	static constexpr std::string_view kBindingLayoutId = "ForwardOpaque";

	BEGIN_SHADER_PARAMETER_STRUCT(FParameters, )
		SHADER_PARAMETER_CBUFFER_NAMED(PerView, PerViewConstantBufferData, PerViewConstantBufferData)
		SHADER_PARAMETER_CBUFFER_NAMED(PerObjectVS, PerObjectVSConstantBufferData, PerObjectVSConstantBufferData)
	END_SHADER_PARAMETER_STRUCT()
};

IMPLEMENT_GLOBAL_SHADER(ForwardOpaqueVS, "Passes/Forward/ForwardLitVS.hlsl", "main", Vertex);

class ForwardOpaquePS final : public TGlobalShader<ForwardOpaquePS>
{
  public:
	static constexpr std::string_view kShaderName = "ForwardOpaquePS";
	static constexpr std::string_view kShaderPackageName = "ForwardOpaque";
	static constexpr std::string_view kBindingLayoutId = "ForwardOpaque";

	BEGIN_SHADER_PARAMETER_STRUCT(FParameters, )
		SHADER_PARAMETER_CBUFFER_NAMED(PerFrame, PerFrameConstantBufferData, PerFrameConstantBufferData)
		SHADER_PARAMETER_CBUFFER_NAMED(PerView, PerViewConstantBufferData, PerViewConstantBufferData)
		SHADER_PARAMETER_CBUFFER_NAMED(PerObjectPS, PerObjectPSConstantBufferData, PerObjectPSConstantBufferData)
		SHADER_PARAMETER_TEXTURE(Texture2D, TextureBaseColor)
		SHADER_PARAMETER_TEXTURE(Texture2D, TextureNormal)
		SHADER_PARAMETER_TEXTURE(Texture2D, TextureMetallicRoughness)
		SHADER_PARAMETER_TEXTURE(Texture2D, TextureOcclusion)
		SHADER_PARAMETER_TEXTURE(Texture2D, TextureEmissive)
		SHADER_PARAMETER_TEXTURE(Texture2D, ShadowMap0)
		SHADER_PARAMETER_TEXTURE(Texture2D, ShadowMap1)
		SHADER_PARAMETER_TEXTURE(Texture2D, ShadowMap2)
		SHADER_PARAMETER_TEXTURE(Texture2D, ShadowMap3)
		SHADER_PARAMETER_SHARED_SAMPLER(SamplerAniso16xWrap)
		SHADER_PARAMETER_SHARED_SAMPLER(SamplerLinearNoMipClamp)
	END_SHADER_PARAMETER_STRUCT()
};

IMPLEMENT_GLOBAL_SHADER(ForwardOpaquePS, "Passes/Forward/ForwardLitPS.hlsl", "main", Pixel);
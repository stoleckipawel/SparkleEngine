#include "PCH.h"

#include "Resources/RenderConstantBufferData.h"
#include "Shaders/Authoring/GlobalShader.h"

#include <cstdint>
#include <string_view>

PassParameterLayout BuildForwardOpaqueShaderPackageBindingLayout()
{
	PassParameterLayout layout("ForwardOpaque");
	layout.Add<RenderTarget>("BackBuffer", ShaderStageVisibility::AllGraphics);
	layout.Add<DepthTarget>("MainDepth", ShaderStageVisibility::AllGraphics);
	layout.Add<ReadTexture>("ShadowMap0", ShaderStageVisibility::Pixel);
	layout.Add<ReadTexture>("ShadowMap1", ShaderStageVisibility::Pixel);
	layout.Add<ReadTexture>("ShadowMap2", ShaderStageVisibility::Pixel);
	layout.Add<ReadTexture>("ShadowMap3", ShaderStageVisibility::Pixel);
	layout.Add<UniformData<PerFrameConstantBufferData>>("PerFrame", ShaderStageVisibility::AllGraphics);
	layout.Add<UniformData<PerViewConstantBufferData>>("PerView", ShaderStageVisibility::AllGraphics);
	layout.Add<SamplerSet>("SamplerTable", ShaderStageVisibility::Pixel);
	layout.Add<UniformData<PerObjectVSConstantBufferData>>("PerObjectVS", ShaderStageVisibility::Vertex);
	layout.Add<UniformData<PerObjectPSConstantBufferData>>("PerObjectPS", ShaderStageVisibility::Pixel);
	layout.Add<ReadTexture>("MaterialTextures", ShaderStageVisibility::Pixel, 5u);
	return layout;
}

void RegisterForwardOpaqueShaders() noexcept
{
}

class ForwardOpaqueVS final : public TGlobalShader<ForwardOpaqueVS>
{
  public:
	static constexpr std::string_view kShaderName = "ForwardOpaqueVS";
	static constexpr std::string_view kShaderPackageName = "ForwardOpaque";
	static constexpr std::string_view kBindingLayoutId = "ForwardOpaque";
	static PassParameterLayout BuildPackageBindingLayout() { return BuildForwardOpaqueShaderPackageBindingLayout(); }

	BEGIN_SHADER_PARAMETER_STRUCT(FParameters, )
		SHADER_PARAMETER(std::uint32_t, PerViewConstantBufferData)
		SHADER_PARAMETER(std::uint32_t, PerObjectVSConstantBufferData)
	END_SHADER_PARAMETER_STRUCT()
};

IMPLEMENT_GLOBAL_SHADER(ForwardOpaqueVS, "Passes/Forward/ForwardLitVS.hlsl", "main", Vertex);

class ForwardOpaquePS final : public TGlobalShader<ForwardOpaquePS>
{
  public:
	static constexpr std::string_view kShaderName = "ForwardOpaquePS";
	static constexpr std::string_view kShaderPackageName = "ForwardOpaque";
	static constexpr std::string_view kBindingLayoutId = "ForwardOpaque";
	static PassParameterLayout BuildPackageBindingLayout() { return BuildForwardOpaqueShaderPackageBindingLayout(); }

	BEGIN_SHADER_PARAMETER_STRUCT(FParameters, )
		SHADER_PARAMETER(std::uint32_t, PerFrameConstantBufferData)
		SHADER_PARAMETER(std::uint32_t, PerViewConstantBufferData)
		SHADER_PARAMETER(std::uint32_t, PerObjectPSConstantBufferData)
		SHADER_PARAMETER_TEXTURE(Texture2D, TextureBaseColor)
		SHADER_PARAMETER_TEXTURE(Texture2D, TextureNormal)
		SHADER_PARAMETER_TEXTURE(Texture2D, TextureMetallicRoughness)
		SHADER_PARAMETER_TEXTURE(Texture2D, TextureOcclusion)
		SHADER_PARAMETER_TEXTURE(Texture2D, TextureEmissive)
		SHADER_PARAMETER_TEXTURE(Texture2D, ShadowMap0)
		SHADER_PARAMETER_TEXTURE(Texture2D, ShadowMap1)
		SHADER_PARAMETER_TEXTURE(Texture2D, ShadowMap2)
		SHADER_PARAMETER_TEXTURE(Texture2D, ShadowMap3)
		SHADER_PARAMETER_SAMPLER(SamplerState, SamplerAniso16xWrap)
		SHADER_PARAMETER_SAMPLER(SamplerState, SamplerLinearNoMipClamp)
	END_SHADER_PARAMETER_STRUCT()
};

IMPLEMENT_GLOBAL_SHADER(ForwardOpaquePS, "Passes/Forward/ForwardLitPS.hlsl", "main", Pixel);
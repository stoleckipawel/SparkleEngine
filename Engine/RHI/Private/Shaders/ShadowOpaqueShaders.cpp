#include "PCH.h"

#include "Resources/RenderConstantBufferData.h"
#include "Shaders/Authoring/GlobalShader.h"

#include <cstdint>
#include <string_view>

PassParameterLayout BuildShadowOpaqueShaderPackageBindingLayout()
{
	PassParameterLayout layout("ShadowOpaque");
	layout.Add<RenderTarget>("ShadowColor", ShaderStageVisibility::AllGraphics);
	layout.Add<DepthTarget>("ShadowDepth", ShaderStageVisibility::AllGraphics);
	layout.Add<UniformData<PerFrameConstantBufferData>>("PerFrame", ShaderStageVisibility::AllGraphics);
	layout.Add<UniformData<PerViewConstantBufferData>>("PerView", ShaderStageVisibility::AllGraphics);
	layout.Add<UniformData<PerObjectVSConstantBufferData>>("PerObjectVS", ShaderStageVisibility::Vertex);
	return layout;
}

void RegisterShadowOpaqueShaders() noexcept
{
}

class ShadowOpaqueVS final : public TGlobalShader<ShadowOpaqueVS>
{
  public:
	static constexpr std::string_view kShaderName = "ShadowOpaqueVS";
	static constexpr std::string_view kShaderPackageName = "ShadowOpaque";
	static constexpr std::string_view kBindingLayoutId = "ShadowOpaque";
	static PassParameterLayout BuildPackageBindingLayout() { return BuildShadowOpaqueShaderPackageBindingLayout(); }

	BEGIN_SHADER_PARAMETER_STRUCT(FParameters, )
		SHADER_PARAMETER(std::uint32_t, PerViewConstantBufferData)
		SHADER_PARAMETER(std::uint32_t, PerObjectVSConstantBufferData)
	END_SHADER_PARAMETER_STRUCT()
};

IMPLEMENT_GLOBAL_SHADER(ShadowOpaqueVS, "Passes/Shadow/ShadowDepthVS.hlsl", "main", Vertex);

class ShadowOpaquePS final : public TGlobalShader<ShadowOpaquePS>
{
  public:
	static constexpr std::string_view kShaderName = "ShadowOpaquePS";
	static constexpr std::string_view kShaderPackageName = "ShadowOpaque";
	static constexpr std::string_view kBindingLayoutId = "ShadowOpaque";
	static PassParameterLayout BuildPackageBindingLayout() { return BuildShadowOpaqueShaderPackageBindingLayout(); }

	BEGIN_SHADER_PARAMETER_STRUCT(FParameters, )
	END_SHADER_PARAMETER_STRUCT()
};

IMPLEMENT_GLOBAL_SHADER(ShadowOpaquePS, "Passes/Shadow/ShadowDepthPS.hlsl", "main", Pixel);
#include "PCH.h"

#include "Shaders/Authoring/GlobalShader.h"

#include "Resources/RenderConstantBufferData.h"

#include <string_view>

void RegisterShadowOpaqueShaders() noexcept
{
}

class ShadowOpaqueVS final : public TGlobalShader<ShadowOpaqueVS>
{
  public:
	static constexpr std::string_view kShaderName = "ShadowOpaqueVS";
	static constexpr std::string_view kShaderPackageName = "ShadowOpaque";
	static constexpr std::string_view kBindingLayoutId = "ShadowOpaque";

	BEGIN_SHADER_PARAMETER_STRUCT(FParameters, )
		SHADER_PARAMETER_CBUFFER_NAMED(PerView, PerViewConstantBufferData, PerViewConstantBufferData)
		SHADER_PARAMETER_CBUFFER_NAMED(PerObjectVS, PerObjectVSConstantBufferData, PerObjectVSConstantBufferData)
	END_SHADER_PARAMETER_STRUCT()
};

IMPLEMENT_GLOBAL_SHADER(ShadowOpaqueVS, "Passes/Shadow/ShadowDepthVS.hlsl", "main", Vertex);

class ShadowOpaquePS final : public TGlobalShader<ShadowOpaquePS>
{
  public:
	static constexpr std::string_view kShaderName = "ShadowOpaquePS";
	static constexpr std::string_view kShaderPackageName = "ShadowOpaque";
	static constexpr std::string_view kBindingLayoutId = "ShadowOpaque";

	BEGIN_SHADER_PARAMETER_STRUCT(FParameters, )
	END_SHADER_PARAMETER_STRUCT()
};

IMPLEMENT_GLOBAL_SHADER(ShadowOpaquePS, "Passes/Shadow/ShadowDepthPS.hlsl", "main", Pixel);
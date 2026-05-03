#include "PCH.h"

#include "Shaders/Authoring/GlobalShader.h"

#include "Resources/RenderConstantBufferData.h"

#include <string_view>

void RegisterDeferredLightingShaders() noexcept
{
}

class DeferredLightingCS final : public TGlobalShader<DeferredLightingCS>
{
  public:
	static constexpr std::string_view kShaderName = "DeferredLightingCS";
	static constexpr std::string_view kShaderPackageName = "DeferredLighting";
	static constexpr std::string_view kBindingLayoutId = "DeferredLighting";

	BEGIN_SHADER_PARAMETER_STRUCT(FParameters, )
		SHADER_PARAMETER_UAV_NAMED(RWTexture2D, SceneColor, SceneColorTexture)
		SHADER_PARAMETER_CBUFFER_NAMED(PerFrame, PerFrameConstantBufferData, PerFrameConstantBufferData)
		SHADER_PARAMETER_CBUFFER_NAMED(PerView, PerViewConstantBufferData, PerViewConstantBufferData)
		SHADER_PARAMETER_TEXTURE(Texture2D, GBufferBaseColor)
		SHADER_PARAMETER_TEXTURE(Texture2D, GBufferNormal)
		SHADER_PARAMETER_TEXTURE(Texture2D, GBufferMaterial)
		SHADER_PARAMETER_TEXTURE(Texture2D, GBufferEmissive)
	END_SHADER_PARAMETER_STRUCT()
};

IMPLEMENT_GLOBAL_SHADER(DeferredLightingCS, "Passes/Deferred/DeferredLighting.hlsl", "main", Compute);
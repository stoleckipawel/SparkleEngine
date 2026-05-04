#include "PCH.h"

#include "Shaders/Authoring/GlobalShader.h"

#include "Resources/RenderConstantBufferData.h"

#include <string_view>

void RegisterSkyShaders() noexcept
{
}

class SkyCS final : public TGlobalShader<SkyCS>
{
  public:
	static constexpr std::string_view kShaderName = "SkyCS";
	static constexpr std::string_view kShaderPackageName = "Sky";
	static constexpr std::string_view kBindingLayoutId = "Sky";

	BEGIN_SHADER_PARAMETER_STRUCT(FParameters, )
		SHADER_PARAMETER_UAV_NAMED(RWTexture2D, SceneColor, SceneColorTexture)
		SHADER_PARAMETER_CBUFFER_NAMED(PerFrame, PerFrameConstantBufferData, PerFrameConstantBufferData)
		SHADER_PARAMETER_CBUFFER_NAMED(PerView, PerViewConstantBufferData, PerViewConstantBufferData)
		SHADER_PARAMETER_TEXTURE(Texture2D, GBufferDeviceZ)
		SHADER_PARAMETER_TEXTURE(TextureCube, SkyTexture)
		SHADER_PARAMETER_SHARED_SAMPLER(SamplerLinearClamp)
	END_SHADER_PARAMETER_STRUCT()
};

IMPLEMENT_GLOBAL_SHADER(SkyCS, "Passes/Deferred/Sky.hlsl", "main", Compute);
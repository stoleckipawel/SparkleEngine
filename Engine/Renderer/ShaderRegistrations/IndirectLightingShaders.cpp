#include "PCH.h"

#include "RendererShaderPackages.h"
#include "Shaders/Authoring/GlobalShader.h"

#include <string_view>

void RegisterIndirectLightingShaders() noexcept {}

class IndirectLightingCS final : public TGlobalShader<IndirectLightingCS>
{
  public:
	static constexpr std::string_view kShaderName = "IndirectLightingCS";
	static constexpr std::string_view kShaderPackageName = RendererShaderPackages::IndirectLighting;
	static constexpr std::string_view kBindingLayoutId = RendererShaderPackages::IndirectLighting;

	BEGIN_SHADER_PARAMETER_STRUCT(FParameters, )
	SHADER_PARAMETER_UAV_NAMED(RWTexture2D, IndirectDiffuse, IndirectDiffuseTexture)
	SHADER_PARAMETER_UAV_NAMED(RWTexture2D, IndirectSpecular, IndirectSpecularTexture)
	SHADER_PARAMETER_UAV_NAMED(RWTexture2D, IndirectSubsurface, IndirectSubsurfaceTexture)
	SHADER_PARAMETER_TEXTURE(Texture2D, GBufferNormal)
	SHADER_PARAMETER_TEXTURE(Texture2D, GBufferDeviceZ)
	SHADER_PARAMETER_TEXTURE(Texture2D, SkyTexture)
	SHADER_PARAMETER_SHARED_SAMPLER(SamplerLinearNoMipClamp)
	END_SHADER_PARAMETER_STRUCT()
};

IMPLEMENT_GLOBAL_SHADER(IndirectLightingCS, "Passes/Deferred/IndirectLighting.hlsl", "main", Compute);

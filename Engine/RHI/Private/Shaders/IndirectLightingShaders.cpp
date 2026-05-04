#include "PCH.h"

#include "Shaders/Authoring/GlobalShader.h"

#include <string_view>

void RegisterIndirectLightingShaders() noexcept
{
}

class IndirectLightingCS final : public TGlobalShader<IndirectLightingCS>
{
  public:
	static constexpr std::string_view kShaderName = "IndirectLightingCS";
	static constexpr std::string_view kShaderPackageName = "IndirectLighting";
	static constexpr std::string_view kBindingLayoutId = "IndirectLighting";

	BEGIN_SHADER_PARAMETER_STRUCT(FParameters, )
		SHADER_PARAMETER_UAV_NAMED(RWTexture2D, IndirectDiffuse, IndirectDiffuseTexture)
		SHADER_PARAMETER_UAV_NAMED(RWTexture2D, IndirectSpecular, IndirectSpecularTexture)
		SHADER_PARAMETER_UAV_NAMED(RWTexture2D, IndirectSubsurface, IndirectSubsurfaceTexture)
	END_SHADER_PARAMETER_STRUCT()
};

IMPLEMENT_GLOBAL_SHADER(IndirectLightingCS, "Passes/Deferred/IndirectLighting.hlsl", "main", Compute);
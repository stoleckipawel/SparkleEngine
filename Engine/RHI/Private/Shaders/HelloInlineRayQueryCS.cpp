#include "PCH.h"

#include "Shaders/Authoring/GlobalShader.h"

#include <string_view>

class HelloInlineRayQueryCS final : public FComputeShader
{
  public:
	static constexpr std::string_view kShaderName = "HelloInlineRayQueryCS";
	static constexpr std::string_view kShaderPackageName = "HelloInlineRayQuery";
	static constexpr std::string_view kBindingLayoutId = "HelloInlineRayQuery";
	static constexpr CookedShaderPackageFeatureFlags kPackageFeatures =
	    CookedShaderPackageFeatureFlags::UsesInlineRayQuery | CookedShaderPackageFeatureFlags::UsesAccelerationStructure;

	BEGIN_SHADER_PARAMETER_STRUCT(FParameters, )
	SHADER_PARAMETER_ACCELERATION_STRUCTURE(SceneAccelerationStructure)
	SHADER_PARAMETER_UAV(RWTexture2D, OutputTexture)
	END_SHADER_PARAMETER_STRUCT()
};

IMPLEMENT_GLOBAL_SHADER(HelloInlineRayQueryCS, "HelloWorld/HelloInlineRayQueryCS.hlsl", "main", Compute);

void RegisterHelloInlineRayQueryCSShader() noexcept
{
	(void)AutoRegisterGlobalShader_HelloInlineRayQueryCS;
}

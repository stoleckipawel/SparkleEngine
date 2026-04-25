#include "PCH.h"

#include "Shaders/Authoring/GlobalShader.h"

#include <string_view>

PassParameterLayout BuildComputeClearShaderPackageBindingLayout()
{
	PassParameterLayout layout("ComputeClear");
	layout.Add<RWTexture>("Output", ShaderStageVisibility::Compute);
	return layout;
}

void RegisterComputeClearShaders() noexcept
{
}

class ComputeClearCS final : public TGlobalShader<ComputeClearCS>
{
  public:
	static constexpr std::string_view kShaderName = "ComputeClearCS";
	static constexpr std::string_view kShaderPackageName = "ComputeClear";
	static constexpr std::string_view kBindingLayoutId = "ComputeClear";
	static PassParameterLayout BuildPackageBindingLayout() { return BuildComputeClearShaderPackageBindingLayout(); }

	BEGIN_SHADER_PARAMETER_STRUCT(FParameters, )
		SHADER_PARAMETER_RDG_TEXTURE_UAV(RWTexture2D, OutputTexture)
	END_SHADER_PARAMETER_STRUCT()
};

IMPLEMENT_GLOBAL_SHADER(ComputeClearCS, "Passes/Compute/ComputeClearColorCS.hlsl", "main", Compute);
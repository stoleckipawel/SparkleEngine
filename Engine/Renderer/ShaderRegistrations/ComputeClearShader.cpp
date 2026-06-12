#include "PCH.h"

#include "Shaders/Authoring/GlobalShader.h"

#include <string_view>

void RegisterComputeClearShaders() noexcept {}

class ComputeClearCS final : public TGlobalShader<ComputeClearCS>
{
  public:
	static constexpr std::string_view kShaderName = "ComputeClearCS";
	static constexpr std::string_view kShaderPackageName = "ComputeClear";
	static constexpr std::string_view kBindingLayoutId = "ComputeClear";

	BEGIN_SHADER_PARAMETER_STRUCT(FParameters, )
	SHADER_PARAMETER_UAV_NAMED(RWTexture2D, Output, OutputTexture)
	END_SHADER_PARAMETER_STRUCT()
};

IMPLEMENT_GLOBAL_SHADER(ComputeClearCS, "Passes/Compute/ComputeClearColorCS.hlsl", "main", Compute);
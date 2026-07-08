#include "PCH.h"

#include "RendererShaderPackages.h"
#include "Shaders/Authoring/GlobalShader.h"

class LinearUpscaleCS final : public TGlobalShader<LinearUpscaleCS>
{
  public:
	BEGIN_SHADER_PARAMETER_STRUCT(FParameters, )
	SHADER_PARAMETER_UAV(RWTexture2D, ScalingOutputColor)
	SHADER_PARAMETER_TEXTURE(Texture2D, ScalingInputColor)
	SHADER_PARAMETER_SHARED_SAMPLER(SamplerLinearClamp)
	END_SHADER_PARAMETER_STRUCT()
};

IMPLEMENT_GLOBAL_SHADER_IN_PACKAGE(
    LinearUpscaleCS,
    RendererShaderPackages::LinearUpscale,
    "Passes/Presentation/LinearUpscale.hlsl",
    "main",
    Compute);

#include "PCH.h"

#include "DirectShadowSignalShaderParameters.h"
#include "RendererShaderPackages.h"
#include "Shaders/Authoring/GlobalShader.h"

class DirectShadowSignalNoRayQueryCS final : public TGlobalShader<DirectShadowSignalNoRayQueryCS>
{
  public:
	BEGIN_SHADER_PARAMETER_STRUCT(FParameters, )
	DIRECT_SHADOW_SIGNAL_COMMON_SHADER_PARAMETERS()
	END_SHADER_PARAMETER_STRUCT()
};

IMPLEMENT_GLOBAL_SHADER_IN_PACKAGE(
    DirectShadowSignalNoRayQueryCS,
    RendererShaderPackages::DirectShadowSignalNoRayQuery,
    "Passes/Deferred/DirectShadowSignalNoRayQuery.hlsl",
    "main",
    Compute);

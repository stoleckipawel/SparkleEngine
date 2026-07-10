#include "PCH.h"

#include "DirectShadowSignalShaderParameters.h"
#include "RendererShaderPackages.h"
#include "Renderer/Private/RayTracing/RayTracingShaderFeatureFlags.h"
#include "Shaders/Authoring/GlobalShader.h"

class DirectShadowSignalCS final : public TGlobalShader<DirectShadowSignalCS>
{
  public:
	static constexpr CookedShaderPackageFeatureFlags kPackageFeatures = RayTracingShaderFeatureFlags::DescriptorRayQuery;

	BEGIN_SHADER_PARAMETER_STRUCT(FParameters, )
	DIRECT_SHADOW_SIGNAL_RAY_QUERY_SHADER_PARAMETERS()
	SHADER_PARAMETER_ACCELERATION_STRUCTURE(SceneTlas)
	END_SHADER_PARAMETER_STRUCT()
};

IMPLEMENT_GLOBAL_SHADER_IN_PACKAGE(
    DirectShadowSignalCS,
    RendererShaderPackages::DirectShadowSignal,
    "Passes/Deferred/DirectShadowSignal.hlsl",
    "main",
    Compute);

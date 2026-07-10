#include "PCH.h"

#include "DirectShadowSignalShaderParameters.h"
#include "RendererShaderPackages.h"
#include "Renderer/Private/RayTracing/RayTracingShaderFeatureFlags.h"
#include "Shaders/Authoring/GlobalShader.h"

class DirectShadowSignalDeviceAddressCS final : public TGlobalShader<DirectShadowSignalDeviceAddressCS>
{
  public:
	static constexpr CookedShaderPackageFeatureFlags kPackageFeatures = RayTracingShaderFeatureFlags::DeviceAddressRayQuery;

	BEGIN_SHADER_PARAMETER_STRUCT(FParameters, )
	DIRECT_SHADOW_SIGNAL_RAY_QUERY_SHADER_PARAMETERS()
	END_SHADER_PARAMETER_STRUCT()
};

IMPLEMENT_GLOBAL_SHADER_IN_PACKAGE(
    DirectShadowSignalDeviceAddressCS,
    RendererShaderPackages::DirectShadowSignalDeviceAddress,
    "Passes/Deferred/DirectShadowSignalDeviceAddress.hlsl",
    "main",
    Compute);

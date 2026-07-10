#include "PCH.h"
#include "RendererShaderPackages.h"
#include "Shaders/Authoring/GlobalShader.h"
#include "RestirIndirectShaderParameters.h"

class RestirIndirectTemporalCS final : public TGlobalShader<RestirIndirectTemporalCS>
{
  public:
	static constexpr CookedShaderPackageFeatureFlags kPackageFeatures = RestirIndirectShaderParameters::PackageFeatures;

	BEGIN_SHADER_PARAMETER_STRUCT(FParameters, )
	SHADER_PARAMETER_UAV(RWTexture2D, TemporalReservoirSampleTexture)
	SHADER_PARAMETER_UAV(RWTexture2D, TemporalReservoirWeightTexture)
	SHADER_PARAMETER_TEXTURE(Texture2D, PreviousReservoirSampleTexture)
	SHADER_PARAMETER_TEXTURE(Texture2D, PreviousReservoirWeightTexture)
	SHADER_PARAMETER_TEXTURE(Texture2D, PreviousReservoirSurfaceTexture)
	SHADER_PARAMETER_TEXTURE(Texture2D, GBufferMotionVector)
	SHADER_PARAMETER_CBUFFER_NAMED(PerTemporal, PerTemporalConstantBufferData, PerTemporalConstantBufferData)
	RESTIR_INDIRECT_SHADER_PARAMETERS()
	END_SHADER_PARAMETER_STRUCT()
};

IMPLEMENT_GLOBAL_SHADER_IN_PACKAGE(
    RestirIndirectTemporalCS,
    RendererShaderPackages::RestirIndirectTemporal,
    "Passes/RayTracing/RestirIndirectTemporal.hlsl",
    "main",
    Compute);

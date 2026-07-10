#include "PCH.h"
#include "RendererShaderPackages.h"
#include "Shaders/Authoring/GlobalShader.h"
#include "RestirIndirectShaderParameters.h"

class RestirIndirectSpatialCS final : public TGlobalShader<RestirIndirectSpatialCS>
{
  public:
	static constexpr CookedShaderPackageFeatureFlags kPackageFeatures = RestirIndirectShaderParameters::PackageFeatures;

	BEGIN_SHADER_PARAMETER_STRUCT(FParameters, )
	SHADER_PARAMETER_TEXTURE(Texture2D, TemporalReservoirSampleTexture)
	SHADER_PARAMETER_TEXTURE(Texture2D, TemporalReservoirWeightTexture)
	SHADER_PARAMETER_UAV(RWTexture2D, CurrentReservoirSampleTexture)
	SHADER_PARAMETER_UAV(RWTexture2D, CurrentReservoirWeightTexture)
	SHADER_PARAMETER_UAV(RWTexture2D, CurrentReservoirSurfaceTexture)
	RESTIR_INDIRECT_SHADER_PARAMETERS()
	END_SHADER_PARAMETER_STRUCT()
};

IMPLEMENT_GLOBAL_SHADER_IN_PACKAGE(
    RestirIndirectSpatialCS,
    RendererShaderPackages::RestirIndirectSpatial,
    "Passes/RayTracing/RestirIndirectSpatial.hlsl",
    "main",
    Compute);

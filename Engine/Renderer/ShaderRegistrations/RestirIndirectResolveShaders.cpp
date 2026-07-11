#include "PCH.h"
#include "RendererShaderPackages.h"
#include "Shaders/Authoring/GlobalShader.h"
#include "RestirIndirectShaderParameters.h"

class RestirIndirectResolveCS final : public TGlobalShader<RestirIndirectResolveCS>
{
  public:
	static constexpr CookedShaderPackageFeatureFlags kPackageFeatures = RestirIndirectShaderParameters::PackageFeatures;

	BEGIN_SHADER_PARAMETER_STRUCT(FParameters, )
	SHADER_PARAMETER_TEXTURE(Texture2D, CurrentReservoirSampleTexture)
	SHADER_PARAMETER_TEXTURE(Texture2D, CurrentReservoirWeightTexture)
	SHADER_PARAMETER_UAV(RWTexture2D, IndirectDiffuse)
	SHADER_PARAMETER_UAV(RWTexture2D, IndirectSpecular)
	SHADER_PARAMETER_UAV(RWTexture2D, RayReconstructionDiffuseAlbedo)
	SHADER_PARAMETER_UAV(RWTexture2D, RayReconstructionSpecularAlbedo)
	SHADER_PARAMETER_UAV(RWTexture2D, RayReconstructionRoughness)
	SHADER_PARAMETER_UAV(RWTexture2D, RayReconstructionSpecularHitDistance)
	RESTIR_INDIRECT_SHADER_PARAMETERS()
	END_SHADER_PARAMETER_STRUCT()
};

IMPLEMENT_GLOBAL_SHADER_IN_PACKAGE(
    RestirIndirectResolveCS,
    RendererShaderPackages::RestirIndirectResolve,
    "Passes/RayTracing/RestirIndirectResolve.hlsl",
    "main",
    Compute);

#include "PCH.h"

#include "DirectLightReservoirShaderParameters.h"
#include "RendererShaderPackages.h"
#include "Shaders/Authoring/GlobalShader.h"

class DirectLightReservoirSpatialCS final : public TGlobalShader<DirectLightReservoirSpatialCS>
{
  public:
	BEGIN_SHADER_PARAMETER_STRUCT(FParameters, )
	SHADER_PARAMETER_TEXTURE_NAMED(Texture2D, TemporalReservoirSample, TemporalReservoirSampleTexture)
	SHADER_PARAMETER_TEXTURE_NAMED(Texture2D, TemporalReservoirWeight, TemporalReservoirWeightTexture)
	SHADER_PARAMETER_UAV_NAMED(RWTexture2D, CurrentReservoirSample, CurrentReservoirSampleTexture)
	SHADER_PARAMETER_UAV_NAMED(RWTexture2D, CurrentReservoirWeight, CurrentReservoirWeightTexture)
	SHADER_PARAMETER_UAV_NAMED(RWTexture2D, CurrentReservoirSurface, CurrentReservoirSurfaceTexture)
	DIRECT_LIGHT_RESERVOIR_FRAME_SHADER_PARAMETERS
	SHADER_PARAMETER_CBUFFER_NAMED(PerTemporal, PerTemporalConstantBufferData, PerTemporalConstantBufferData)
	DIRECT_LIGHT_RESERVOIR_SURFACE_AND_LIGHTING_SHADER_PARAMETERS
	END_SHADER_PARAMETER_STRUCT()
};

IMPLEMENT_GLOBAL_SHADER_IN_PACKAGE(
    DirectLightReservoirSpatialCS,
    RendererShaderPackages::DirectLightReservoirSpatial,
    "Passes/Deferred/DirectLightReservoirSpatial.hlsl",
    "main",
    Compute);

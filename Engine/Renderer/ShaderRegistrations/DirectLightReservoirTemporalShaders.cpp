#include "PCH.h"

#include "DirectLightReservoirShaderParameters.h"
#include "RendererShaderPackages.h"
#include "Shaders/Authoring/GlobalShader.h"

class DirectLightReservoirTemporalCS final : public TGlobalShader<DirectLightReservoirTemporalCS>
{
  public:
	BEGIN_SHADER_PARAMETER_STRUCT(FParameters, )
	SHADER_PARAMETER_UAV_NAMED(RWTexture2D, TemporalReservoirSample, TemporalReservoirSampleTexture)
	SHADER_PARAMETER_UAV_NAMED(RWTexture2D, TemporalReservoirWeight, TemporalReservoirWeightTexture)
	SHADER_PARAMETER_TEXTURE_NAMED(Texture2D, PreviousReservoirSample, PreviousReservoirSampleTexture)
	SHADER_PARAMETER_TEXTURE_NAMED(Texture2D, PreviousReservoirWeight, PreviousReservoirWeightTexture)
	SHADER_PARAMETER_TEXTURE_NAMED(Texture2D, PreviousReservoirSurface, PreviousReservoirSurfaceTexture)
	DIRECT_LIGHT_RESERVOIR_FRAME_SHADER_PARAMETERS
	SHADER_PARAMETER_CBUFFER_NAMED(PerTemporal, PerTemporalConstantBufferData, PerTemporalConstantBufferData)
	DIRECT_LIGHT_RESERVOIR_SURFACE_AND_LIGHTING_SHADER_PARAMETERS
	SHADER_PARAMETER_TEXTURE(Texture2D, GBufferMotionVector)
	END_SHADER_PARAMETER_STRUCT()
};

IMPLEMENT_GLOBAL_SHADER_IN_PACKAGE(
    DirectLightReservoirTemporalCS,
    RendererShaderPackages::DirectLightReservoirTemporal,
    "Passes/Deferred/DirectLightReservoirTemporal.hlsl",
    "main",
    Compute);

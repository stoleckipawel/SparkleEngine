#include "PCH.h"

#include "RendererShaderPackages.h"
#include "Shaders/Authoring/GlobalShader.h"

#include "ShaderData/RenderConstantBufferData.h"
#include "ShaderData/RenderViewLightingData.h"

class DirectLightReservoirTemporalCS final : public TGlobalShader<DirectLightReservoirTemporalCS>
{
  public:
	BEGIN_SHADER_PARAMETER_STRUCT(FParameters, )
	SHADER_PARAMETER_UAV_NAMED(RWTexture2D, TemporalReservoirSample, TemporalReservoirSampleTexture)
	SHADER_PARAMETER_UAV_NAMED(RWTexture2D, TemporalReservoirWeight, TemporalReservoirWeightTexture)
	SHADER_PARAMETER_TEXTURE_NAMED(Texture2D, PreviousReservoirSample, PreviousReservoirSampleTexture)
	SHADER_PARAMETER_TEXTURE_NAMED(Texture2D, PreviousReservoirWeight, PreviousReservoirWeightTexture)
	SHADER_PARAMETER_TEXTURE_NAMED(Texture2D, PreviousReservoirSurface, PreviousReservoirSurfaceTexture)
	SHADER_PARAMETER_CBUFFER_NAMED(PerFrame, PerFrameConstantBufferData, PerFrameConstantBufferData)
	SHADER_PARAMETER_CBUFFER_NAMED(PerView, PerViewConstantBufferData, PerViewConstantBufferData)
	SHADER_PARAMETER_CBUFFER_NAMED(PerTemporal, PerTemporalConstantBufferData, PerTemporalConstantBufferData)
	SHADER_PARAMETER_CBUFFER_NAMED(ViewLighting, ViewLighting, ViewLightingData)
	SHADER_PARAMETER_RDG_BUFFER_SRV(DirectionalLightConstantBufferData, DirectionalLights)
	SHADER_PARAMETER_RDG_BUFFER_SRV(PointLightConstantBufferData, PointLights)
	SHADER_PARAMETER_RDG_BUFFER_SRV(SpotLightConstantBufferData, SpotLights)
	SHADER_PARAMETER_RDG_BUFFER_SRV(RectLightConstantBufferData, RectLights)
	SHADER_PARAMETER_TEXTURE(Texture2D, GBufferBaseColor)
	SHADER_PARAMETER_TEXTURE(Texture2D, GBufferNormal)
	SHADER_PARAMETER_TEXTURE(Texture2D, GBufferMaterial)
	SHADER_PARAMETER_TEXTURE(Texture2D, GBufferSubsurface)
	SHADER_PARAMETER_TEXTURE(Texture2D, SceneDepth)
	SHADER_PARAMETER_TEXTURE(Texture2D, GBufferMotionVector)
	END_SHADER_PARAMETER_STRUCT()
};

class DirectLightReservoirSpatialCS final : public TGlobalShader<DirectLightReservoirSpatialCS>
{
  public:
	BEGIN_SHADER_PARAMETER_STRUCT(FParameters, )
	SHADER_PARAMETER_TEXTURE_NAMED(Texture2D, TemporalReservoirSample, TemporalReservoirSampleTexture)
	SHADER_PARAMETER_TEXTURE_NAMED(Texture2D, TemporalReservoirWeight, TemporalReservoirWeightTexture)
	SHADER_PARAMETER_UAV_NAMED(RWTexture2D, CurrentReservoirSample, CurrentReservoirSampleTexture)
	SHADER_PARAMETER_UAV_NAMED(RWTexture2D, CurrentReservoirWeight, CurrentReservoirWeightTexture)
	SHADER_PARAMETER_UAV_NAMED(RWTexture2D, CurrentReservoirSurface, CurrentReservoirSurfaceTexture)
	SHADER_PARAMETER_CBUFFER_NAMED(PerFrame, PerFrameConstantBufferData, PerFrameConstantBufferData)
	SHADER_PARAMETER_CBUFFER_NAMED(PerView, PerViewConstantBufferData, PerViewConstantBufferData)
	SHADER_PARAMETER_CBUFFER_NAMED(ViewLighting, ViewLighting, ViewLightingData)
	SHADER_PARAMETER_RDG_BUFFER_SRV(DirectionalLightConstantBufferData, DirectionalLights)
	SHADER_PARAMETER_RDG_BUFFER_SRV(PointLightConstantBufferData, PointLights)
	SHADER_PARAMETER_RDG_BUFFER_SRV(SpotLightConstantBufferData, SpotLights)
	SHADER_PARAMETER_RDG_BUFFER_SRV(RectLightConstantBufferData, RectLights)
	SHADER_PARAMETER_TEXTURE(Texture2D, GBufferBaseColor)
	SHADER_PARAMETER_TEXTURE(Texture2D, GBufferNormal)
	SHADER_PARAMETER_TEXTURE(Texture2D, GBufferMaterial)
	SHADER_PARAMETER_TEXTURE(Texture2D, GBufferSubsurface)
	SHADER_PARAMETER_TEXTURE(Texture2D, SceneDepth)
	END_SHADER_PARAMETER_STRUCT()
};

IMPLEMENT_GLOBAL_SHADER_IN_PACKAGE(
    DirectLightReservoirTemporalCS,
    RendererShaderPackages::DirectLightReservoirTemporal,
    "Passes/Deferred/DirectLightReservoirTemporal.hlsl",
    "main",
    Compute);

IMPLEMENT_GLOBAL_SHADER_IN_PACKAGE(
    DirectLightReservoirSpatialCS,
    RendererShaderPackages::DirectLightReservoirSpatial,
    "Passes/Deferred/DirectLightReservoirSpatial.hlsl",
    "main",
    Compute);

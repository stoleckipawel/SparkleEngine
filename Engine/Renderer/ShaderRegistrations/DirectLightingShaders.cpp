#include "PCH.h"

#include "RendererShaderPackages.h"
#include "Shaders/Authoring/GlobalShader.h"

#include "Resources/RenderConstantBufferData.h"
#include "Resources/RenderViewLightingData.h"

class DirectLightingCS final : public TGlobalShader<DirectLightingCS>
{
  public:
	BEGIN_SHADER_PARAMETER_STRUCT(FParameters, )
	SHADER_PARAMETER_UAV_NAMED(RWTexture2D, DirectDiffuse, DirectDiffuseTexture)
	SHADER_PARAMETER_UAV_NAMED(RWTexture2D, DirectSpecular, DirectSpecularTexture)
	SHADER_PARAMETER_UAV_NAMED(RWTexture2D, DirectSubsurface, DirectSubsurfaceTexture)
	SHADER_PARAMETER_TEXTURE_NAMED(Texture2D, ShadowVisibilitySignal, ShadowVisibilitySignalTexture)
	SHADER_PARAMETER_TEXTURE_NAMED(Texture2D, CurrentReservoirSample, CurrentReservoirSampleTexture)
	SHADER_PARAMETER_TEXTURE_NAMED(Texture2D, CurrentReservoirWeight, CurrentReservoirWeightTexture)
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
	SHADER_PARAMETER_TEXTURE(Texture2D, GBufferDeviceZ)
	END_SHADER_PARAMETER_STRUCT()
};

IMPLEMENT_GLOBAL_SHADER_IN_PACKAGE(
    DirectLightingCS,
    RendererShaderPackages::DirectLighting,
    "Passes/Deferred/DirectLighting.hlsl",
    "main",
    Compute);

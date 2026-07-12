#include "PCH.h"

#include "RendererShaderPackages.h"
#include "Renderer/Private/RayTracing/Effects/Shadows/RayTracedShadowUniformData.h"
#include "Renderer/Private/RayTracing/RayTracingHitData.h"
#include "Renderer/Private/RayTracing/RayTracingShaderFeatureFlags.h"
#include "Renderer/Private/SceneData/MaterialTextureTableCapability.h"
#include "Shaders/Authoring/GlobalShader.h"
#include "ShaderData/RenderConstantBufferData.h"
#include "ShaderData/RenderViewLightingData.h"

class DirectShadowSignalCS final : public TGlobalShader<DirectShadowSignalCS>
{
  public:
	static constexpr CookedShaderPackageFeatureFlags kPackageFeatures = RayTracingShaderFeatureFlags::DescriptorRayQuery;

	BEGIN_SHADER_PARAMETER_STRUCT(FParameters, )
	SHADER_PARAMETER_UAV_NAMED(RWTexture2D, ShadowVisibilitySignal, ShadowVisibilitySignalTexture)
	SHADER_PARAMETER_TEXTURE_NAMED(Texture2D, CurrentReservoirSample, CurrentReservoirSampleTexture)
	SHADER_PARAMETER_TEXTURE_NAMED(Texture2D, CurrentReservoirWeight, CurrentReservoirWeightTexture)
	SHADER_PARAMETER_CBUFFER_NAMED(PerFrame, PerFrameConstantBufferData, PerFrameConstantBufferData)
	SHADER_PARAMETER_CBUFFER_NAMED(PerView, PerViewConstantBufferData, PerViewConstantBufferData)
	SHADER_PARAMETER_CBUFFER_NAMED(PerTemporal, PerTemporalConstantBufferData, PerTemporalConstantBufferData)
	SHADER_PARAMETER_CBUFFER_NAMED(ViewLighting, ViewLighting, ViewLightingData)
	SHADER_PARAMETER_RDG_BUFFER_SRV(DirectionalLightConstantBufferData, DirectionalLights)
	SHADER_PARAMETER_RDG_BUFFER_SRV(PointLightConstantBufferData, PointLights)
	SHADER_PARAMETER_RDG_BUFFER_SRV(SpotLightConstantBufferData, SpotLights)
	SHADER_PARAMETER_RDG_BUFFER_SRV(RectLightConstantBufferData, RectLights)
	SHADER_PARAMETER_TEXTURE(Texture2D, SceneDepth)
	SHADER_PARAMETER_CBUFFER_NAMED(RayTracedShadows, RayTracedShadowUniformData, RayTracedShadowUniformData)
	SHADER_PARAMETER_RDG_BUFFER_SRV(RayTracingHitVertex, RayTracingHitVertices)
	SHADER_PARAMETER_RDG_BUFFER_SRV(uint32_t, RayTracingHitIndices)
	SHADER_PARAMETER_RDG_BUFFER_SRV(RayTracingHitInstance, RayTracingHitInstances)
	SHADER_PARAMETER_RDG_BUFFER_SRV(RayTracingHitMaterial, RayTracingHitMaterials)
	SHADER_PARAMETER_TEXTURE_ARRAY(Texture2D, MaterialTextureTable, MaterialTextureTableFixedCapacity)
	SHADER_PARAMETER_SAMPLER(SamplerState, MaterialTextureSampler)
	SHADER_PARAMETER_TEXTURE(Texture2D, GBufferNormal)
	SHADER_PARAMETER_ACCELERATION_STRUCTURE(SceneTlas)
	END_SHADER_PARAMETER_STRUCT()
};

IMPLEMENT_GLOBAL_SHADER_IN_PACKAGE(
    DirectShadowSignalCS,
    RendererShaderPackages::DirectShadowSignal,
    "Passes/Deferred/DirectShadowSignal.hlsl",
    "main",
    Compute);

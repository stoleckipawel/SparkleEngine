#include "PCH.h"

#include "RendererShaderPackages.h"
#include "Shaders/Authoring/GlobalShader.h"

#include "Resources/RenderConstantBufferData.h"
#include "Resources/RenderViewLightingData.h"
#include "Renderer/Private/RayTracing/Effects/Shadows/RayTracedShadowUniformData.h"
#include "Renderer/Private/RayTracing/RayTracingHitData.h"
#include "Renderer/Private/SceneData/MaterialTextureTableCapability.h"

class DirectShadowSignalNoRayQueryCS final : public TGlobalShader<DirectShadowSignalNoRayQueryCS>
{
  public:
	BEGIN_SHADER_PARAMETER_STRUCT(FParameters, )
	SHADER_PARAMETER_UAV_NAMED(RWTexture2D, ShadowVisibilitySignal, ShadowVisibilitySignalTexture)
	SHADER_PARAMETER_UAV_NAMED(RWTexture2D, ShadowLightSample, ShadowLightSampleTexture)
	SHADER_PARAMETER_CBUFFER_NAMED(PerFrame, PerFrameConstantBufferData, PerFrameConstantBufferData)
	SHADER_PARAMETER_CBUFFER_NAMED(PerView, PerViewConstantBufferData, PerViewConstantBufferData)
	SHADER_PARAMETER_CBUFFER_NAMED(ViewLighting, ViewLighting, ViewLightingData)
	SHADER_PARAMETER_RDG_BUFFER_SRV(DirectionalLightConstantBufferData, DirectionalLights)
	SHADER_PARAMETER_RDG_BUFFER_SRV(PointLightConstantBufferData, PointLights)
	SHADER_PARAMETER_RDG_BUFFER_SRV(SpotLightConstantBufferData, SpotLights)
	SHADER_PARAMETER_RDG_BUFFER_SRV(RectLightConstantBufferData, RectLights)
	SHADER_PARAMETER_TEXTURE(Texture2D, GBufferNormal)
	SHADER_PARAMETER_TEXTURE(Texture2D, GBufferDeviceZ)
	END_SHADER_PARAMETER_STRUCT()
};

class DirectShadowSignalCS final : public TGlobalShader<DirectShadowSignalCS>
{
  public:
	static constexpr CookedShaderPackageFeatureFlags kPackageFeatures =
	    CookedShaderPackageFeatureFlags::UsesAccelerationStructure | CookedShaderPackageFeatureFlags::UsesInlineRayQuery |
	    CookedShaderPackageFeatureFlags::UsesDescriptorIndexing;

	BEGIN_SHADER_PARAMETER_STRUCT(FParameters, )
	SHADER_PARAMETER_UAV_NAMED(RWTexture2D, ShadowVisibilitySignal, ShadowVisibilitySignalTexture)
	SHADER_PARAMETER_UAV_NAMED(RWTexture2D, ShadowLightSample, ShadowLightSampleTexture)
	SHADER_PARAMETER_ACCELERATION_STRUCTURE(SceneTlas)
	SHADER_PARAMETER_CBUFFER_NAMED(PerFrame, PerFrameConstantBufferData, PerFrameConstantBufferData)
	SHADER_PARAMETER_CBUFFER_NAMED(PerView, PerViewConstantBufferData, PerViewConstantBufferData)
	SHADER_PARAMETER_CBUFFER_NAMED(ViewLighting, ViewLighting, ViewLightingData)
	SHADER_PARAMETER_CBUFFER_NAMED(RayTracedShadows, RayTracedShadowUniformData, RayTracedShadowUniformData)
	SHADER_PARAMETER_RDG_BUFFER_SRV(DirectionalLightConstantBufferData, DirectionalLights)
	SHADER_PARAMETER_RDG_BUFFER_SRV(PointLightConstantBufferData, PointLights)
	SHADER_PARAMETER_RDG_BUFFER_SRV(SpotLightConstantBufferData, SpotLights)
	SHADER_PARAMETER_RDG_BUFFER_SRV(RectLightConstantBufferData, RectLights)
	SHADER_PARAMETER_RDG_BUFFER_SRV(RayTracingHitVertex, RayTracingHitVertices)
	SHADER_PARAMETER_RDG_BUFFER_SRV(uint32_t, RayTracingHitIndices)
	SHADER_PARAMETER_RDG_BUFFER_SRV(RayTracingHitInstance, RayTracingHitInstances)
	SHADER_PARAMETER_RDG_BUFFER_SRV(RayTracingHitMaterial, RayTracingHitMaterials)
	SHADER_PARAMETER_TEXTURE_ARRAY(Texture2D, MaterialTextureTable, MaterialTextureTableFixedCapacity)
	SHADER_PARAMETER_SAMPLER(SamplerState, MaterialTextureSampler)
	SHADER_PARAMETER_TEXTURE(Texture2D, GBufferNormal)
	SHADER_PARAMETER_TEXTURE(Texture2D, GBufferDeviceZ)
	END_SHADER_PARAMETER_STRUCT()
};

class DirectShadowSignalDeviceAddressCS final : public TGlobalShader<DirectShadowSignalDeviceAddressCS>
{
  public:
	static constexpr CookedShaderPackageFeatureFlags kPackageFeatures =
	    CookedShaderPackageFeatureFlags::UsesAccelerationStructure |
	    CookedShaderPackageFeatureFlags::UsesAccelerationStructureDeviceAddress |
	    CookedShaderPackageFeatureFlags::UsesInlineRayQuery |
	    CookedShaderPackageFeatureFlags::UsesDescriptorIndexing;

	BEGIN_SHADER_PARAMETER_STRUCT(FParameters, )
	SHADER_PARAMETER_UAV_NAMED(RWTexture2D, ShadowVisibilitySignal, ShadowVisibilitySignalTexture)
	SHADER_PARAMETER_UAV_NAMED(RWTexture2D, ShadowLightSample, ShadowLightSampleTexture)
	SHADER_PARAMETER_CBUFFER_NAMED(PerFrame, PerFrameConstantBufferData, PerFrameConstantBufferData)
	SHADER_PARAMETER_CBUFFER_NAMED(PerView, PerViewConstantBufferData, PerViewConstantBufferData)
	SHADER_PARAMETER_CBUFFER_NAMED(ViewLighting, ViewLighting, ViewLightingData)
	SHADER_PARAMETER_CBUFFER_NAMED(RayTracedShadows, RayTracedShadowUniformData, RayTracedShadowUniformData)
	SHADER_PARAMETER_RDG_BUFFER_SRV(DirectionalLightConstantBufferData, DirectionalLights)
	SHADER_PARAMETER_RDG_BUFFER_SRV(PointLightConstantBufferData, PointLights)
	SHADER_PARAMETER_RDG_BUFFER_SRV(SpotLightConstantBufferData, SpotLights)
	SHADER_PARAMETER_RDG_BUFFER_SRV(RectLightConstantBufferData, RectLights)
	SHADER_PARAMETER_RDG_BUFFER_SRV(RayTracingHitVertex, RayTracingHitVertices)
	SHADER_PARAMETER_RDG_BUFFER_SRV(uint32_t, RayTracingHitIndices)
	SHADER_PARAMETER_RDG_BUFFER_SRV(RayTracingHitInstance, RayTracingHitInstances)
	SHADER_PARAMETER_RDG_BUFFER_SRV(RayTracingHitMaterial, RayTracingHitMaterials)
	SHADER_PARAMETER_TEXTURE_ARRAY(Texture2D, MaterialTextureTable, MaterialTextureTableFixedCapacity)
	SHADER_PARAMETER_SAMPLER(SamplerState, MaterialTextureSampler)
	SHADER_PARAMETER_TEXTURE(Texture2D, GBufferNormal)
	SHADER_PARAMETER_TEXTURE(Texture2D, GBufferDeviceZ)
	END_SHADER_PARAMETER_STRUCT()
};

IMPLEMENT_GLOBAL_SHADER_IN_PACKAGE(
    DirectShadowSignalNoRayQueryCS,
    RendererShaderPackages::DirectShadowSignalNoRayQuery,
    "Passes/Deferred/DirectShadowSignalNoRayQuery.hlsl",
    "main",
    Compute);

IMPLEMENT_GLOBAL_SHADER_IN_PACKAGE(
    DirectShadowSignalCS,
    RendererShaderPackages::DirectShadowSignal,
    "Passes/Deferred/DirectShadowSignal.hlsl",
    "main",
    Compute);

IMPLEMENT_GLOBAL_SHADER_IN_PACKAGE(
    DirectShadowSignalDeviceAddressCS,
    RendererShaderPackages::DirectShadowSignalDeviceAddress,
    "Passes/Deferred/DirectShadowSignalDeviceAddress.hlsl",
    "main",
    Compute);

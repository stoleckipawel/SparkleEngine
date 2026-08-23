#include "PCH.h"

#include "RendererShaderPackages.h"
#include "Renderer/Private/RayTracing/Effects/Shadows/RayTracedShadowUniformData.h"
#include "Renderer/Private/RayTracing/RayTracingHitData.h"
#include "Renderer/Private/RayTracing/RayTracingShaderFeatureFlags.h"
#include "Renderer/Private/Scene/Materials/MaterialTextureTableCapability.h"
#include "Shaders/Authoring/GlobalShader.h"
#include "ShaderData/ViewUniformData.h"
#include "ShaderData/ViewCameraUniformData.h"
#include "ShaderData/ViewTemporalUniformData.h"
#include "ShaderData/LightGpuData.h"
#include "ShaderData/SceneLightingUniformData.h"

class DirectShadowSignalDeviceAddressCS final : public TGlobalShader<DirectShadowSignalDeviceAddressCS>
{
public:
	static constexpr CookedShaderPackageFeatureFlags kPackageFeatures = RayTracingShaderFeatureFlags::DeviceAddressRayQuery;

	BEGIN_SHADER_PARAMETER_STRUCT(FParameters, )
	SHADER_PARAMETER_UAV_NAMED(RWTexture2D, ShadowVisibilitySignal, ShadowVisibilitySignalTexture)
	SHADER_PARAMETER_TEXTURE_NAMED(Texture2D, CurrentReservoirSample, CurrentReservoirSampleTexture)
	SHADER_PARAMETER_TEXTURE_NAMED(Texture2D, CurrentReservoirWeight, CurrentReservoirWeightTexture)
	SHADER_PARAMETER_CBUFFER_NAMED(View, ViewUniformData, ViewUniformData)
	SHADER_PARAMETER_CBUFFER_NAMED(ViewCamera, ViewCameraUniformData, ViewCameraUniformData)
	SHADER_PARAMETER_CBUFFER_NAMED(ViewTemporal, ViewTemporalUniformData, ViewTemporalUniformData)
	SHADER_PARAMETER_CBUFFER_NAMED(SceneLighting, SceneLighting, SceneLightingUniformData)
	SHADER_PARAMETER_RDG_BUFFER_SRV(DirectionalLightGpuData, DirectionalLights)
	SHADER_PARAMETER_RDG_BUFFER_SRV(PointLightGpuData, PointLights)
	SHADER_PARAMETER_RDG_BUFFER_SRV(SpotLightGpuData, SpotLights)
	SHADER_PARAMETER_RDG_BUFFER_SRV(RectLightGpuData, RectLights)
	SHADER_PARAMETER_TEXTURE(Texture2D, SceneDepth)
	SHADER_PARAMETER_CBUFFER_NAMED(RayTracedShadows, RayTracedShadowUniformData, RayTracedShadowUniformData)
	SHADER_PARAMETER_RDG_BUFFER_SRV(RayTracingHitVertex, RayTracingHitVertices)
	SHADER_PARAMETER_RDG_BUFFER_SRV(uint32_t, RayTracingHitIndices)
	SHADER_PARAMETER_RDG_BUFFER_SRV(RayTracingHitInstance, RayTracingHitInstances)
	SHADER_PARAMETER_RDG_BUFFER_SRV(RayTracingHitMaterial, RayTracingHitMaterials)
	SHADER_PARAMETER_TEXTURE_ARRAY(Texture2D, MaterialTextureTable, MaterialTextureTableFixedCapacity)
	SHADER_PARAMETER_SAMPLER(SamplerState, MaterialTextureSampler)
	SHADER_PARAMETER_TEXTURE(Texture2D, GBufferNormal)
	END_SHADER_PARAMETER_STRUCT()
};

IMPLEMENT_GLOBAL_SHADER_IN_PACKAGE(
    DirectShadowSignalDeviceAddressCS,
    RendererShaderPackages::DirectShadowSignalDeviceAddress,
    "Passes/Deferred/DirectShadowSignalDeviceAddress.hlsl",
    "main",
    Compute);

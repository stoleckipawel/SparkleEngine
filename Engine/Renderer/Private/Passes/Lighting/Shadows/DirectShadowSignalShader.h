#pragma once

#include "ShaderData/SceneLightingUniformData.h"
#include "ShaderData/ViewTemporalUniformData.h"
#include "ShaderData/ViewCameraUniformData.h"
#include "ShaderData/ViewUniformData.h"
#include "ShaderParameters/ShaderParameterStruct.h"
#include "RHI/Public/Shaders/Authoring/GlobalShader.h"
#include "Renderer/Private/RayTracing/RayTracingHitData.h"
#include "Renderer/Private/RayTracing/RayTracingShaderFeatureFlags.h"
#include "Renderer/Private/ShaderData/RayTracingMaterialPayload.h"
#include "Renderer/Private/Scene/Materials/MaterialTextureTableCapability.h"
#include "ShaderData/LightGpuData.h"

#include <cstdint>

class DirectShadowSignalCS final : public GlobalShader<DirectShadowSignalCS>
{
public:
	static constexpr ShaderFeatureFlags kShaderFeatures = RayTracingShaderFeatureFlags::InlineRayQuery;

	BEGIN_SHADER_PARAMETER_STRUCT(Parameters, DirectShadowSignalCS)
	SHADER_PARAMETER_TEXTURE_UAV(RWTexture2D, ShadowVisibilitySignal)
	SHADER_PARAMETER_TEXTURE_SRV(Texture2D, CurrentReservoirSample)
	SHADER_PARAMETER_TEXTURE_SRV(Texture2D, CurrentReservoirWeight)
	SHADER_PARAMETER_CBUFFER(ViewUniformData, View)
	SHADER_PARAMETER_CBUFFER(ViewCameraUniformData, ViewCamera)
	SHADER_PARAMETER_CBUFFER(ViewTemporalUniformData, ViewTemporal)
	SHADER_PARAMETER_CBUFFER(SceneLightingUniformData, SceneLighting)
	SHADER_PARAMETER_BUFFER_SRV(DirectionalLightGpuData, DirectionalLights)
	SHADER_PARAMETER_BUFFER_SRV(PointLightGpuData, PointLights)
	SHADER_PARAMETER_BUFFER_SRV(SpotLightGpuData, SpotLights)
	SHADER_PARAMETER_BUFFER_SRV(RectLightGpuData, RectLights)
	SHADER_PARAMETER_TEXTURE_SRV(Texture2D, SceneDepth)
	SHADER_PARAMETER(std::uint32_t, RayTracedDirectionalShadowsEnabled)
	SHADER_PARAMETER(std::uint32_t, RayTracedLocalLightShadowsEnabled)
	SHADER_PARAMETER(float, RayTracedShadowNormalBias)
	SHADER_PARAMETER(float, RayTracedShadowMaxDistance)
	SHADER_PARAMETER_BUFFER_SRV(RayTracingHitVertex, RayTracingHitVertices)
	SHADER_PARAMETER_BUFFER_SRV(uint32_t, RayTracingHitIndices)
	SHADER_PARAMETER_BUFFER_SRV(RayTracingHitInstance, RayTracingHitInstances)
	SHADER_PARAMETER_BUFFER_SRV(RayTracingHitMaterial, RayTracingHitMaterials)
	SHADER_PARAMETER_TEXTURE_SRV_ARRAY(Texture2D, MaterialTextureTable, MaterialTextureTableFixedCapacity)
	SHADER_PARAMETER_TEXTURE_SRV(Texture2D, GBufferWorldNormal)
	SHADER_PARAMETER_ACCELERATION_STRUCTURE(SceneTlas)
	END_SHADER_PARAMETER_STRUCT()
};

class DirectShadowSignalRGS final : public GlobalShader<DirectShadowSignalRGS>
{
public:
	using Parameters = DirectShadowSignalCS::Parameters;
	static constexpr ShaderFeatureFlags kShaderFeatures = RayTracingShaderFeatureFlags::SceneBindings;
	static constexpr RayTracingShaderMetadata kRayTracingMetadata = kRayTracingMaterialShaderMetadata;
};

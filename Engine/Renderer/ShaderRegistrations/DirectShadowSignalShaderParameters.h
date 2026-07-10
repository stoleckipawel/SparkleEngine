#pragma once

#include "Renderer/Private/RayTracing/Effects/Shadows/RayTracedShadowUniformData.h"
#include "Renderer/Private/RayTracing/RayTracingHitData.h"
#include "Renderer/Private/SceneData/MaterialTextureTableCapability.h"
#include "ShaderData/RenderConstantBufferData.h"
#include "ShaderData/RenderViewLightingData.h"

#define DIRECT_SHADOW_SIGNAL_COMMON_SHADER_PARAMETERS()                                              \
	SHADER_PARAMETER_UAV_NAMED(RWTexture2D, ShadowVisibilitySignal, ShadowVisibilitySignalTexture)   \
	SHADER_PARAMETER_TEXTURE_NAMED(Texture2D, CurrentReservoirSample, CurrentReservoirSampleTexture) \
	SHADER_PARAMETER_TEXTURE_NAMED(Texture2D, CurrentReservoirWeight, CurrentReservoirWeightTexture) \
	SHADER_PARAMETER_CBUFFER_NAMED(PerFrame, PerFrameConstantBufferData, PerFrameConstantBufferData) \
	SHADER_PARAMETER_CBUFFER_NAMED(PerView, PerViewConstantBufferData, PerViewConstantBufferData)    \
	SHADER_PARAMETER_CBUFFER_NAMED(ViewLighting, ViewLighting, ViewLightingData)                     \
	SHADER_PARAMETER_RDG_BUFFER_SRV(DirectionalLightConstantBufferData, DirectionalLights)           \
	SHADER_PARAMETER_RDG_BUFFER_SRV(PointLightConstantBufferData, PointLights)                       \
	SHADER_PARAMETER_RDG_BUFFER_SRV(SpotLightConstantBufferData, SpotLights)                         \
	SHADER_PARAMETER_RDG_BUFFER_SRV(RectLightConstantBufferData, RectLights)                         \
	SHADER_PARAMETER_TEXTURE(Texture2D, SceneDepth)

#define DIRECT_SHADOW_SIGNAL_RAY_QUERY_SHADER_PARAMETERS()                                                   \
	DIRECT_SHADOW_SIGNAL_COMMON_SHADER_PARAMETERS()                                                          \
	SHADER_PARAMETER_CBUFFER_NAMED(RayTracedShadows, RayTracedShadowUniformData, RayTracedShadowUniformData) \
	SHADER_PARAMETER_RDG_BUFFER_SRV(RayTracingHitVertex, RayTracingHitVertices)                              \
	SHADER_PARAMETER_RDG_BUFFER_SRV(uint32_t, RayTracingHitIndices)                                          \
	SHADER_PARAMETER_RDG_BUFFER_SRV(RayTracingHitInstance, RayTracingHitInstances)                           \
	SHADER_PARAMETER_RDG_BUFFER_SRV(RayTracingHitMaterial, RayTracingHitMaterials)                           \
	SHADER_PARAMETER_TEXTURE_ARRAY(Texture2D, MaterialTextureTable, MaterialTextureTableFixedCapacity)       \
	SHADER_PARAMETER_SAMPLER(SamplerState, MaterialTextureSampler)                                           \
	SHADER_PARAMETER_TEXTURE(Texture2D, GBufferNormal)

#pragma once

#include "Renderer/Private/RayTracing/RayTracingShaderFeatureFlags.h"
#include "Renderer/Private/RayTracing/Effects/Shadows/RayTracedShadowUniformData.h"
#include "Renderer/Private/RayTracing/RayTracingHitData.h"
#include "Renderer/Private/SceneData/MaterialTextureTableCapability.h"
#include "ShaderData/RenderConstantBufferData.h"
#include "ShaderData/RenderViewLightingData.h"

namespace RayTracedSurfaceLightingShaderParameters
{
	inline constexpr CookedShaderPackageFeatureFlags PackageFeatures = RayTracingShaderFeatureFlags::DescriptorRayQuery;
}

#define RAY_TRACED_SURFACE_LIGHTING_SHADER_PARAMETERS()                                                      \
	SHADER_PARAMETER_ACCELERATION_STRUCTURE(SceneTlas)                                                       \
	SHADER_PARAMETER_CBUFFER_NAMED(PerFrame, PerFrameConstantBufferData, PerFrameConstantBufferData)         \
	SHADER_PARAMETER_CBUFFER_NAMED(PerView, PerViewConstantBufferData, PerViewConstantBufferData)            \
	SHADER_PARAMETER_CBUFFER_NAMED(ViewLighting, ViewLighting, ViewLightingData)                             \
	SHADER_PARAMETER_CBUFFER_NAMED(RayTracedShadows, RayTracedShadowUniformData, RayTracedShadowUniformData) \
	SHADER_PARAMETER_TEXTURE(Texture2D, GBufferBaseColor)                                                    \
	SHADER_PARAMETER_TEXTURE(Texture2D, GBufferNormal)                                                       \
	SHADER_PARAMETER_TEXTURE(Texture2D, GBufferMaterial)                                                     \
	SHADER_PARAMETER_TEXTURE(Texture2D, SceneDepth)                                                          \
	SHADER_PARAMETER_TEXTURE(Texture2D, SkyTexture)                                                          \
	SHADER_PARAMETER_SAMPLER(SamplerState, SamplerLinearClamp)                                               \
	SHADER_PARAMETER_RDG_BUFFER_SRV(RayTracingHitVertex, RayTracingHitVertices)                              \
	SHADER_PARAMETER_RDG_BUFFER_SRV(uint32_t, RayTracingHitIndices)                                          \
	SHADER_PARAMETER_RDG_BUFFER_SRV(RayTracingHitInstance, RayTracingHitInstances)                           \
	SHADER_PARAMETER_RDG_BUFFER_SRV(RayTracingHitMaterial, RayTracingHitMaterials)                           \
	SHADER_PARAMETER_RDG_BUFFER_SRV(MeshInstanceData, MeshInstances)                                         \
	SHADER_PARAMETER_RDG_BUFFER_SRV(VertexSkinInfluenceData, SkinInfluences)                                 \
	SHADER_PARAMETER_RDG_BUFFER_SRV(JointMatrixData, JointMatrices)                                          \
	SHADER_PARAMETER_RDG_BUFFER_SRV(DirectionalLightConstantBufferData, DirectionalLights)                   \
	SHADER_PARAMETER_RDG_BUFFER_SRV(PointLightConstantBufferData, PointLights)                               \
	SHADER_PARAMETER_RDG_BUFFER_SRV(SpotLightConstantBufferData, SpotLights)                                 \
	SHADER_PARAMETER_RDG_BUFFER_SRV(RectLightConstantBufferData, RectLights)                                 \
	SHADER_PARAMETER_TEXTURE_ARRAY(Texture2D, MaterialTextureTable, MaterialTextureTableFixedCapacity)       \
	SHADER_PARAMETER_SAMPLER(SamplerState, MaterialTextureSampler)

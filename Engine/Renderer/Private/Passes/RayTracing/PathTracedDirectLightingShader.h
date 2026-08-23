#pragma once

#include "Renderer/Public/ShaderParameters/ShaderParameterStruct.h"
#include "RHI/Public/Shaders/Authoring/GlobalShader.h"
#include "Renderer/Private/RayTracing/Effects/PathTracedLighting/PathTracedLightingUniformData.h"
#include "Renderer/Private/RayTracing/Effects/Shadows/RayTracedShadowUniformData.h"
#include "Renderer/Private/RayTracing/RayTracingHitData.h"
#include "Renderer/Private/RayTracing/RayTracingShaderFeatureFlags.h"
#include "Renderer/Private/Scene/Materials/MaterialTextureTableCapability.h"
#include "ShaderData/FrameUniformData.h"
#include "ShaderData/ViewUniformData.h"
#include "ShaderData/ViewCameraUniformData.h"
#include "ShaderData/ViewTemporalUniformData.h"
#include "ShaderData/LightGpuData.h"
#include "ShaderData/SceneLightingUniformData.h"

class PathTracedDirectLightingCS final : public GlobalShader<PathTracedDirectLightingCS>
{
public:
	static constexpr CookedShaderPackageFeatureFlags kPackageFeatures = RayTracingShaderFeatureFlags::InlineRayQuery;
	BEGIN_SHADER_PARAMETER_STRUCT(Parameters, )
	SHADER_PARAMETER_TEXTURE_UAV(RWTexture2D, DirectDiffuse)
	SHADER_PARAMETER_TEXTURE_UAV(RWTexture2D, DirectSpecular)
	SHADER_PARAMETER_TEXTURE_UAV(RWTexture2D, DirectSubsurface)
	SHADER_PARAMETER_ACCELERATION_STRUCTURE(SceneTlas)
	SHADER_PARAMETER_CBUFFER(FrameUniformData, Frame)
	SHADER_PARAMETER_CBUFFER(ViewUniformData, View)
	SHADER_PARAMETER_CBUFFER(ViewCameraUniformData, ViewCamera)
	SHADER_PARAMETER_CBUFFER(ViewTemporalUniformData, ViewTemporal)
	SHADER_PARAMETER_CBUFFER(SceneLightingUniformData, SceneLighting)
	SHADER_PARAMETER_CBUFFER(RayTracedShadowUniformData, RayTracedShadowConstants)
	SHADER_PARAMETER_CBUFFER(PathTracedLightingUniformData, PathTracedLightingConstants)
	SHADER_PARAMETER_TEXTURE_SRV(Texture2D, GBufferBaseColor)
	SHADER_PARAMETER_TEXTURE_SRV(Texture2D, GBufferNormal)
	SHADER_PARAMETER_TEXTURE_SRV(Texture2D, GBufferMaterial)
	SHADER_PARAMETER_TEXTURE_SRV(Texture2D, GBufferSubsurface)
	SHADER_PARAMETER_TEXTURE_SRV(Texture2D, SceneDepth)
	SHADER_PARAMETER_BUFFER_SRV(DirectionalLightGpuData, DirectionalLights)
	SHADER_PARAMETER_BUFFER_SRV(PointLightGpuData, PointLights)
	SHADER_PARAMETER_BUFFER_SRV(SpotLightGpuData, SpotLights)
	SHADER_PARAMETER_BUFFER_SRV(RectLightGpuData, RectLights)
	SHADER_PARAMETER_BUFFER_SRV(RayTracingHitVertex, RayTracingHitVertices)
	SHADER_PARAMETER_BUFFER_SRV(uint32_t, RayTracingHitIndices)
	SHADER_PARAMETER_BUFFER_SRV(RayTracingHitInstance, RayTracingHitInstances)
	SHADER_PARAMETER_BUFFER_SRV(RayTracingHitMaterial, RayTracingHitMaterials)
	SHADER_PARAMETER_TEXTURE_SRV_ARRAY(Texture2D, MaterialTextureTable, MaterialTextureTableFixedCapacity)
	SHADER_PARAMETER_SAMPLER(SamplerState, MaterialTextureSampler)
	END_SHADER_PARAMETER_STRUCT()
};

#pragma once

#include "Frame/Graph/RenderFrameGraphTargets.h"
#include "RayTracing/Effects/Shadows/RayTracedShadowUniformData.h"
#include "Renderer/Public/ShaderParameters/ShaderParameterFields.h"
#include "Renderer/Public/ShaderParameters/ShaderParameterStructBuilder.h"
#include "Scene/Materials/MaterialTextureTableCapability.h"
#include "ShaderData/ViewUniformData.h"
#include "ShaderData/ViewCameraUniformData.h"
#include "ShaderData/ViewTemporalUniformData.h"
#include "ShaderData/LightGpuData.h"
#include "ShaderData/SceneLightingUniformData.h"

struct DirectShadowSignalCommonPassParameters
{
	ShaderRWTexture2D<void> ShadowVisibilitySignal;
	ShaderTexture2D<void> CurrentReservoirSample;
	ShaderTexture2D<void> CurrentReservoirWeight;
	ShaderTexture2D<void> SceneDepth;
	ShaderUniform<ViewUniformData> View;
	ShaderUniform<ViewCameraUniformData> ViewCamera;
	ShaderUniform<ViewTemporalUniformData> ViewTemporal;
	ShaderUniform<SceneLightingUniformData> SceneLighting;
	ShaderBuffer<void> DirectionalLights;
	ShaderBuffer<void> PointLights;
	ShaderBuffer<void> SpotLights;
	ShaderBuffer<void> RectLights;

	template <typename TParameters> static void Describe(ShaderParameterStructBuilder<TParameters>& builder)
	{
		builder.RWTexture(
		    "ShadowVisibilitySignal",
		    static_cast<ShaderRWTexture2D<void> TParameters::*>(&DirectShadowSignalCommonPassParameters::ShadowVisibilitySignal),
		    ShaderStageVisibility::Compute);
		builder.ReadTexture(
		    "CurrentReservoirSample",
		    static_cast<ShaderTexture2D<void> TParameters::*>(&DirectShadowSignalCommonPassParameters::CurrentReservoirSample),
		    ShaderStageVisibility::Compute);
		builder.ReadTexture(
		    "CurrentReservoirWeight",
		    static_cast<ShaderTexture2D<void> TParameters::*>(&DirectShadowSignalCommonPassParameters::CurrentReservoirWeight),
		    ShaderStageVisibility::Compute);
		builder.ReadTexture(
		    "SceneDepth",
		    static_cast<ShaderTexture2D<void> TParameters::*>(&DirectShadowSignalCommonPassParameters::SceneDepth),
		    ShaderStageVisibility::Compute);
		builder.Uniform(
		    "View",
		    static_cast<ShaderUniform<ViewUniformData> TParameters::*>(&DirectShadowSignalCommonPassParameters::View),
		    ShaderStageVisibility::Compute);
		builder.Uniform(
		    "ViewCamera",
		    static_cast<ShaderUniform<ViewCameraUniformData> TParameters::*>(&DirectShadowSignalCommonPassParameters::ViewCamera),
		    ShaderStageVisibility::Compute);
		builder.Uniform(
		    "ViewTemporal",
		    static_cast<ShaderUniform<ViewTemporalUniformData> TParameters::*>(&DirectShadowSignalCommonPassParameters::ViewTemporal),
		    ShaderStageVisibility::Compute);
		builder.Uniform(
		    "SceneLighting",
		    static_cast<ShaderUniform<SceneLightingUniformData> TParameters::*>(&DirectShadowSignalCommonPassParameters::SceneLighting),
		    ShaderStageVisibility::Compute);
		builder.ReadBuffer(
		    "DirectionalLights",
		    static_cast<ShaderBuffer<void> TParameters::*>(&DirectShadowSignalCommonPassParameters::DirectionalLights),
		    ShaderStageVisibility::Compute);
		builder.ReadBuffer(
		    "PointLights",
		    static_cast<ShaderBuffer<void> TParameters::*>(&DirectShadowSignalCommonPassParameters::PointLights),
		    ShaderStageVisibility::Compute);
		builder.ReadBuffer(
		    "SpotLights",
		    static_cast<ShaderBuffer<void> TParameters::*>(&DirectShadowSignalCommonPassParameters::SpotLights),
		    ShaderStageVisibility::Compute);
		builder.ReadBuffer(
		    "RectLights",
		    static_cast<ShaderBuffer<void> TParameters::*>(&DirectShadowSignalCommonPassParameters::RectLights),
		    ShaderStageVisibility::Compute);
	}
};

struct DirectShadowSignalRayQueryPassParameters : DirectShadowSignalCommonPassParameters
{
	ShaderTexture2D<void> GBufferNormal;
	ShaderUniform<RayTracedShadowUniformData> RayTracedShadows;
	ShaderBuffer<void> RayTracingHitVertices;
	ShaderBuffer<void> RayTracingHitIndices;
	ShaderBuffer<void> RayTracingHitInstances;
	ShaderBuffer<void> RayTracingHitMaterials;
	ShaderTexture2DTableSRV<MaterialTextureTableFixedCapacity> MaterialTextureTable;
	ShaderSamplerSet MaterialTextureSampler;

	template <typename TParameters> static void Describe(ShaderParameterStructBuilder<TParameters>& builder)
	{
		DirectShadowSignalCommonPassParameters::Describe(builder);
		builder.ReadTexture(
		    "GBufferNormal",
		    static_cast<ShaderTexture2D<void> TParameters::*>(&DirectShadowSignalRayQueryPassParameters::GBufferNormal),
		    ShaderStageVisibility::Compute);
		builder.Uniform(
		    "RayTracedShadows",
		    static_cast<ShaderUniform<RayTracedShadowUniformData> TParameters::*>(
		        &DirectShadowSignalRayQueryPassParameters::RayTracedShadows),
		    ShaderStageVisibility::Compute);
		builder.ReadBuffer(
		    "RayTracingHitVertices",
		    static_cast<ShaderBuffer<void> TParameters::*>(&DirectShadowSignalRayQueryPassParameters::RayTracingHitVertices),
		    ShaderStageVisibility::Compute);
		builder.ReadBuffer(
		    "RayTracingHitIndices",
		    static_cast<ShaderBuffer<void> TParameters::*>(&DirectShadowSignalRayQueryPassParameters::RayTracingHitIndices),
		    ShaderStageVisibility::Compute);
		builder.ReadBuffer(
		    "RayTracingHitInstances",
		    static_cast<ShaderBuffer<void> TParameters::*>(&DirectShadowSignalRayQueryPassParameters::RayTracingHitInstances),
		    ShaderStageVisibility::Compute);
		builder.ReadBuffer(
		    "RayTracingHitMaterials",
		    static_cast<ShaderBuffer<void> TParameters::*>(&DirectShadowSignalRayQueryPassParameters::RayTracingHitMaterials),
		    ShaderStageVisibility::Compute);
		builder.ReadTexture(
		    "MaterialTextureTable",
		    static_cast<ShaderTexture2DTableSRV<MaterialTextureTableFixedCapacity> TParameters::*>(
		        &DirectShadowSignalRayQueryPassParameters::MaterialTextureTable),
		    ShaderStageVisibility::Compute);
		builder.Sampler(
		    "MaterialTextureSampler",
		    static_cast<ShaderSamplerSet TParameters::*>(&DirectShadowSignalRayQueryPassParameters::MaterialTextureSampler),
		    ShaderStageVisibility::Compute);
	}
};

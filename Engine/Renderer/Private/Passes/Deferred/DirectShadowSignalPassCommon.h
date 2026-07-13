#pragma once

#include "Frame/Targets/FrameRenderTargets.h"
#include "RayTracing/Effects/Shadows/RayTracedShadowUniformData.h"
#include "Renderer/Public/ShaderParameters/ShaderParameterFields.h"
#include "Renderer/Public/ShaderParameters/ShaderParameterStructBuilder.h"
#include "SceneData/MaterialTextureTableCapability.h"
#include "ShaderData/RenderConstantBufferData.h"
#include "ShaderData/RenderViewLightingData.h"

class FrameGraphBuilder;
struct DirectShadowSignalResources;
struct FrameContext;
struct PassRuntimeServices;
struct RenderViewData;

struct DirectShadowSignalCommonPassParameters
{
	ShaderRWTexture2D<void> ShadowVisibilitySignal;
	ShaderTexture2D<void> CurrentReservoirSample;
	ShaderTexture2D<void> CurrentReservoirWeight;
	ShaderTexture2D<void> SceneDepth;
	ShaderUniform<PerFrameConstantBufferData> PerFrame;
	ShaderUniform<PerViewConstantBufferData> PerView;
	ShaderUniform<PerTemporalConstantBufferData> PerTemporal;
	ShaderUniform<ViewLightingData> ViewLighting;
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
		    "PerFrame",
		    static_cast<ShaderUniform<PerFrameConstantBufferData> TParameters::*>(&DirectShadowSignalCommonPassParameters::PerFrame),
		    ShaderStageVisibility::Compute);
		builder.Uniform(
		    "PerView",
		    static_cast<ShaderUniform<PerViewConstantBufferData> TParameters::*>(&DirectShadowSignalCommonPassParameters::PerView),
		    ShaderStageVisibility::Compute);
		builder.Uniform(
		    "PerTemporal",
		    static_cast<ShaderUniform<PerTemporalConstantBufferData> TParameters::*>(&DirectShadowSignalCommonPassParameters::PerTemporal),
		    ShaderStageVisibility::Compute);
		builder.Uniform(
		    "ViewLighting",
		    static_cast<ShaderUniform<ViewLightingData> TParameters::*>(&DirectShadowSignalCommonPassParameters::ViewLighting),
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

namespace DirectShadowSignalPassCommon
{
	void DeclareResources(
	    FrameGraphBuilder& builder,
	    FrameGraphTextureHandle sceneDepth,
	    const DirectShadowSignalResources& shadowSignals,
	    FrameGraphBufferHandle directionalLights,
	    FrameGraphBufferHandle pointLights,
	    FrameGraphBufferHandle spotLights,
	    FrameGraphBufferHandle rectLights,
	    DirectShadowSignalCommonPassParameters& parameters);

	void DeclareRayQueryResources(
	    FrameGraphBuilder& builder,
	    FrameGraphTextureHandle sceneDepth,
	    const GBufferRenderTargets& gbuffer,
	    const DirectShadowSignalResources& shadowSignals,
	    FrameGraphBufferHandle directionalLights,
	    FrameGraphBufferHandle pointLights,
	    FrameGraphBufferHandle spotLights,
	    FrameGraphBufferHandle rectLights,
	    FrameGraphBufferHandle hitVertices,
	    FrameGraphBufferHandle hitIndices,
	    FrameGraphBufferHandle hitInstances,
	    FrameGraphBufferHandle hitMaterials,
	    DirectShadowSignalRayQueryPassParameters& parameters);

	void SetParameters(
	    DirectShadowSignalCommonPassParameters& parameters,
	    const FrameContext& frame,
	    const RenderViewData& viewData,
	    const PassRuntimeServices& passRuntimeServices);

	void SetRayQueryParameters(
	    DirectShadowSignalRayQueryPassParameters& parameters,
	    const FrameContext& frame,
	    const RenderViewData& viewData,
	    const PassRuntimeServices& passRuntimeServices,
	    bool hasSceneTlas);
}

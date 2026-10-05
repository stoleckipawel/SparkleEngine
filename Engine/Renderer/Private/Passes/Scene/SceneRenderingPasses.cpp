#include "../../PCH.h"
#include "Passes/Scene/SceneRenderingPasses.h"

#include "Frame/Graph/RenderFrameGraphSettings.h"
#include "Frame/Graph/RenderFrameGraphResources.h"
#include "Passes/Lighting/RealTimePathTracerProducts.h"
#include "Passes/Visualization/LightingVisualization.h"
#include "Passes/Lighting/RealTimePathTracerPasses.h"
#include "Passes/Lighting/ReferencePathTracer/ReferencePathTracerPasses.h"
#include "Passes/PostProcessing/Exposure/ExposurePasses.h"
#include "Passes/Presentation/Upscaling/SceneUpscalingPasses.h"
#include "Passes/Scene/SceneDenoisingPasses.h"
#include "Passes/Scene/SceneVisualizationPasses.h"

std::uint64_t GetSceneRenderingTopologyIdentity(RenderViewMode viewMode, const RenderFrameGraphSettings& settings) noexcept
{
	return viewMode == RenderViewMode::ReferencePathTracer ? 0u : GetRealTimePathTracerTopologyIdentity(settings);
}

bool PrepareSceneRenderingProducts(
    RenderViewMode viewMode,
    const RenderFrameGraphSettings& settings,
    const RenderFrameGraphResources& resources,
    ViewportFrameProducts& products) noexcept
{
	if (viewMode != RenderViewMode::ReferencePathTracer && !PrepareRealTimePathTracerProducts(settings, products))
	{
		return false;
	}
	return PrepareLightingVisualizationProducts(viewMode, resources, products);
}

void AddSceneRenderingPasses(
    FrameGraphBuilder& builder,
    const RenderFrameGraphSettings& settings,
    RenderViewMode viewMode,
    RenderRayTracingScene& rayTracingScene,
    GpuMeshCache& gpuMeshCache,
    RendererImageProviderStack& imageProviders,
    ReferencePathTracerSession& referencePathTracerSession,
    RenderFrameGraphResources& resources)
{
	if (viewMode != RenderViewMode::ReferencePathTracer && !PrepareRealTimePathTracerProducts(settings, resources.ViewportProducts))
	{
		return;
	}
	if (viewMode == RenderViewMode::ReferencePathTracer)
	{
		AddReferencePathTracerPasses(builder, settings, referencePathTracerSession, resources);
	}
	else
	{
		AddRealTimePathTracerPasses(builder, settings, rayTracingScene, gpuMeshCache, resources);
	}

	AddExposurePasses(builder, settings, resources);
	AddSceneVisualizationPasses(builder, settings.RenderExtent, viewMode, resources);
	AddSceneDenoisingPasses(builder, settings, imageProviders, resources);
	AddSceneUpscalingPasses(builder, settings, viewMode, imageProviders, resources);
}

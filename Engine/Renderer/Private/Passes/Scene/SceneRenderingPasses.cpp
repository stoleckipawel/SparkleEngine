#include "../../PCH.h"
#include "Passes/Scene/SceneRenderingPasses.h"

#include "FrameGraph/Builder/FrameGraphBuilder.h"
#include "Frame/Graph/RenderFrameGraphSettings.h"
#include "View/RenderView.h"
#include "Frame/Graph/RenderFrameGraphResources.h"
#include "Passes/Lighting/RealTimePathTracerProducts.h"
#include "Passes/Visualization/LightingVisualization.h"
#include "Passes/Lighting/RealTimePathTracerPasses.h"
#include "Passes/Lighting/ReferencePathTracer/ReferencePathTracerPasses.h"
#include "Passes/PostProcessing/Exposure/ExposurePasses.h"
#include "Passes/Presentation/Upscaling/SceneUpscalingPasses.h"
#include "Passes/Lighting/Restir/Reconstruction/RestirRayReconstruction.h"
#include "Passes/Scene/SceneVisualizationPasses.h"

std::uint64_t GetSceneRenderingTopologyIdentity(RenderViewMode viewMode) noexcept
{
	const std::uint64_t rendererIdentity = viewMode == RenderViewMode::ReferencePathTracer ? 0u : GetRealTimePathTracerTopologyIdentity();
	return (static_cast<std::uint64_t>(viewMode) << 8u) | rendererIdentity;
}

bool PrepareSceneRenderingProducts(
    const RenderView& view,
    const RenderFrameGraphResources& resources,
    ViewportFrameProducts& products) noexcept
{
	if (view.viewMode != RenderViewMode::ReferencePathTracer && !PrepareRealTimePathTracerProducts(view.viewMode, products))
	{
		return false;
	}
	return PrepareLightingVisualizationProducts(view.viewMode, resources, products);
}

void AddSceneRenderingPasses(
    FrameGraphBuilder& builder,
    const RenderFrameGraphSettings& settings,
    RenderRayTracingScene& rayTracingScene,
    GpuMeshCache& gpuMeshCache,
    RendererImageProviderStack& imageProviders,
    ReferencePathTracerSession& referencePathTracerSession,
    RenderFrameGraphResources& resources)
{
	if (builder.GetViewMode() != RenderViewMode::ReferencePathTracer
	    && !PrepareRealTimePathTracerProducts(builder.GetViewMode(), resources.ViewportProducts))
	{
		return;
	}
	if (builder.GetViewMode() == RenderViewMode::ReferencePathTracer)
	{
		AddReferencePathTracerPasses(builder, settings, referencePathTracerSession, resources);
	}
	else
	{
		AddRealTimePathTracerPasses(builder, settings, rayTracingScene, gpuMeshCache, resources);
	}

	AddExposurePasses(builder, settings, resources);
	AddSceneVisualizationPasses(builder, settings.RenderExtent, resources);
	AddRestirRayReconstructionPass(builder, settings.RenderExtent, imageProviders, resources);
	AddSceneUpscalingPasses(builder, settings, imageProviders, resources);
}

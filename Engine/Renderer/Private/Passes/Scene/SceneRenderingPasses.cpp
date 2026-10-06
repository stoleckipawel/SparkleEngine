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
#include "Passes/Presentation/PresentationPolicy.h"

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
    const ViewportRenderRequest& viewport,
    RenderRayTracingScene& rayTracingScene,
    GpuMeshCache& gpuMeshCache,
    RendererImageProviderStack& imageProviders,
    ReferencePathTracerSession& referencePathTracerSession,
    RenderFrameGraphResources& resources)
{
	if (viewport.ViewMode != RenderViewMode::ReferencePathTracer
	    && !PrepareRealTimePathTracerProducts(viewport.ViewMode, resources.ViewportProducts))
	{
		return;
	}
	if (viewport.ViewMode == RenderViewMode::ReferencePathTracer)
	{
		AddReferencePathTracerPasses(builder, settings, referencePathTracerSession, resources);
	}
	else
	{
		AddRealTimePathTracerPasses(builder, settings, rayTracingScene, gpuMeshCache, resources);
	}

	AddExposurePasses(builder, settings, resources);
	AddSceneVisualizationPasses(builder, settings.RenderExtent, viewport.ViewMode, resources);
	if (viewport.ViewMode == RenderViewMode::Lit)
	{
		AddRestirRayReconstructionPass(builder, settings.RenderExtent, imageProviders, resources);
	}
	AddSceneUpscalingPasses(builder, settings, ResolveSceneUpscalingMethod(viewport.ViewMode), imageProviders, resources);
}

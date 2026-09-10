#include "../../PCH.h"
#include "Frame/Graph/BuildRenderFrameGraph.h"

#include "Frame/Graph/RenderFrameGraphResourceBindings.h"
#include "Passes/GBuffer/GBuffer.h"
#include "Passes/Lighting/Lighting.h"
#include "Passes/Lighting/ReferencePathTracer/ReferencePathTracerComposition.h"
#include "Passes/PostProcessing/Exposure.h"
#include "Passes/PostProcessing/PostProcessing.h"
#include "Passes/RayTracing/RayTracingScene.h"
#include "FrameGraph/Builder/FrameGraphBuilder.h"

static void AddLitPasses(
    FrameGraphBuilder& builder,
    const RenderFrameGraphSettings& settings,
    GpuMeshCache& gpuMeshCache,
    RenderRayTracingScene& rayTracingScene,
    IRayReconstructionProvider* rayReconstructionProvider,
    RenderFrameGraphResources& resources)
{
	AddGBufferMeshPasses(builder, gpuMeshCache, rayTracingScene, settings.RenderExtent, resources);
	AddLightingPasses(builder, rayTracingScene, settings.RenderExtent, resources);
	AddExposurePass(builder, settings, resources);
	AddLightingReconstructionPasses(builder, settings.RenderExtent, settings.OutputExtent, rayReconstructionProvider, resources);
}

RenderFrameGraphResources BuildRenderFrameGraph(
    FrameGraphBuilder& builder,
    const RenderFrameGraphSettings& settings,
    GpuMeshCache& gpuMeshCache,
    RenderRayTracingScene& rayTracingScene,
    IUpscalerProvider* upscalerProvider,
    IRayReconstructionProvider* rayReconstructionProvider)
{
	RenderFrameGraphResources resources = {};
	CreateRenderFrameGraphResources(builder, settings, resources);
	AddRayTracingScenePasses(builder, rayTracingScene, resources);
	if (settings.ViewMode == RenderViewMode::ReferencePathTracer)
	{
		AddReferencePathTracerPasses(builder, settings, resources);
	}
	else
	{
		AddLitPasses(builder, settings, gpuMeshCache, rayTracingScene, rayReconstructionProvider, resources);
	}

	AddPostProcessingPasses(builder, settings, upscalerProvider, resources);

	return resources;
}

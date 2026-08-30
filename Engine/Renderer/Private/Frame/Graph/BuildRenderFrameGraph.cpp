#include "../../PCH.h"
#include "Frame/Graph/BuildRenderFrameGraph.h"

#include "Frame/Graph/RenderFrameGraphResourceBindings.h"
#include "Passes/GBuffer/GBuffer.h"
#include "Passes/Lighting/Lighting.h"
#include "Passes/PostProcessing/PostProcessing.h"
#include "Passes/RayTracing/RayTracingScene.h"
#include "FrameGraph/Builder/FrameGraphBuilder.h"

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
	AddGBufferMeshPasses(builder, gpuMeshCache, rayTracingScene, settings.RenderExtent, resources);
	AddLightingPasses(builder, rayTracingScene, settings.RenderExtent, resources);
	AddPreReconstructionPostProcessingPasses(builder, settings, resources);
	AddLightingReconstructionPasses(builder, settings.RenderExtent, settings.OutputExtent, rayReconstructionProvider, resources);

	AddPostProcessingPasses(builder, settings, upscalerProvider, resources);

	return resources;
}

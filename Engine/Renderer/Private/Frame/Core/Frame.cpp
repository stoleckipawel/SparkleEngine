#include "../../PCH.h"
#include "Frame/Core/Frame.h"

#include "Frame/Core/FrameSceneResources.h"
#include "Frame/Deferred/GBuffer.h"
#include "Frame/Lighting/Lighting.h"
#include "Frame/PostProcessing/PostProcessing.h"
#include "Frame/RayTracing/RayTracingScene.h"
#include "FrameGraph/Builder/FrameGraphBuilder.h"

FrameBuildResult BuildFrame(
    FrameGraphBuilder& builder,
    const FrameBuildSettings& settings,
    GpuMeshCache& gpuMeshCache,
    RenderRayTracingScene& rayTracingScene,
    IUpscalerProvider* upscalerProvider,
    IRayReconstructionProvider* rayReconstructionProvider)
{
	FrameAssemblyResourceLayout resources = {};
	CreateFrameSceneResources(builder, settings, resources);
	AddRaytracingScenePasses(builder, rayTracingScene, resources);
	AddGBufferPasses(builder, gpuMeshCache, settings.RenderExtent, resources);
	AddLightingPasses(builder, settings.RenderExtent, resources);
	AddPreReconstructionPostProcessingPasses(builder, settings, resources);
	AddLightingReconstructionPasses(builder, settings.RenderExtent, settings.OutputExtent, rayReconstructionProvider, resources);

	AddPostProcessingPasses(builder, settings, upscalerProvider, resources);

	return FrameBuildResult{.Resources = resources};
}

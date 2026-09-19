#include "../../PCH.h"
#include "Passes/Lighting/LightingPasses.h"

#include "Frame/Graph/RenderFrameGraphSettings.h"
#include "Passes/Lighting/RealTimePathTracerPasses.h"
#include "Passes/Lighting/ReferencePathTracer/ReferencePathTracerPasses.h"
#include "Passes/PostProcessing/ExposurePasses.h"
#include "Passes/Presentation/SceneUpscaling.h"

void AddLightingPasses(
    FrameGraphBuilder& builder,
    const RenderFrameGraphSettings& settings,
    RenderViewMode viewMode,
    RenderRayTracingScene& rayTracingScene,
    GpuMeshCache& gpuMeshCache,
    RendererImageProviderStack& imageProviders,
    ReferencePathTracerSession& referencePathTracerSession,
    RenderFrameGraphResources& resources)
{
	if (viewMode == RenderViewMode::ReferencePathTracer)
	{
		AddReferencePathTracerPasses(builder, settings, referencePathTracerSession, resources);
	}
	else
	{
		AddRealTimePathTracerPasses(builder, settings, rayTracingScene, gpuMeshCache, imageProviders, resources);
	}

	AddExposurePasses(builder, settings, resources);
	AddSceneUpscalingPass(builder, settings, imageProviders, resources);
}

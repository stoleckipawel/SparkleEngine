#include "../../PCH.h"
#include "Passes/Lighting/RealTimePathTracerPasses.h"

#include "Frame/Graph/RenderFrameGraphSettings.h"
#include "Passes/GBuffer/GBufferPasses.h"
#include "Passes/Lighting/RealTimeLightingPasses.h"
#include "Passes/Lighting/RealTimePathTracerProducts.h"
#include "Passes/Lighting/Restir/RestirRayReconstruction.h"
#include "Providers/RendererImageProviderStack.h"

void AddRealTimePathTracerPasses(
    FrameGraphBuilder& builder,
    const RenderFrameGraphSettings& settings,
    RenderRayTracingScene& rayTracingScene,
    GpuMeshCache& gpuMeshCache,
    RendererImageProviderStack& imageProviders,
    RenderFrameGraphResources& resources)
{
	AddGBufferPasses(builder, gpuMeshCache, rayTracingScene, settings.RenderExtent, resources);
	AddRealTimeLightingPasses(builder, rayTracingScene, settings.RenderExtent, resources);
	AddRestirRayReconstructionPass(builder, settings, imageProviders, resources);
	PublishRealTimePathTracerProducts(resources);
}

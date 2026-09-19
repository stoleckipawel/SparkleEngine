#include "../../PCH.h"
#include "Passes/Lighting/RealTimePathTracerPasses.h"

#include "Frame/Graph/RenderFrameGraphSettings.h"
#include "Passes/GBuffer/GBufferPasses.h"
#include "Passes/Lighting/LightingComposite.h"
#include "Passes/Lighting/LightingRenderTargets.h"
#include "Passes/Lighting/LightingTargetClear.h"
#include "Passes/Lighting/RealTimePathTracerProducts.h"
#include "Passes/Lighting/Restir/RestirLightingPasses.h"
#include "Passes/Lighting/Restir/RestirRayReconstruction.h"
#include "Passes/Lighting/Sky/Sky.h"
#include "Providers/RendererImageProviderStack.h"

void AddRealTimePathTracerPasses(
    FrameGraphBuilder& builder,
    const RenderFrameGraphSettings& settings,
    RenderRayTracingScene& rayTracingScene,
    GpuMeshCache& gpuMeshCache,
    RendererImageProviderStack& imageProviders,
    RenderFrameGraphResources& resources)
{
	AddGBufferPasses(builder, settings.RenderExtent, gpuMeshCache, rayTracingScene, resources);

	CreateRealTimeLightingRenderTargets(builder, settings.RenderExtent, resources);
	AddLightingTargetClearPass(builder, resources);
	AddRestirLightingPasses(builder, settings.RenderExtent, rayTracingScene, resources);
	AddLightingCompositePass(builder, settings.RenderExtent, resources);
	AddSkyPass(builder, settings.RenderExtent, resources);

	AddRestirRayReconstructionPass(builder, settings, imageProviders, resources);

	PublishRealTimePathTracerProducts(resources);
}

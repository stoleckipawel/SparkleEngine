#include "../../PCH.h"
#include "Passes/Lighting/RealTimePathTracerPasses.h"

#include "Frame/Graph/RenderFrameGraphSettings.h"
#include "Passes/GBuffer/GBufferPasses.h"
#include "Passes/Lighting/LightingComposite.h"
#include "Passes/Lighting/RealTimePathTracerProducts.h"
#include "Passes/Lighting/Restir/RestirLightingPasses.h"
#include "Passes/Lighting/Sky/Sky.h"
#include "Frame/Graph/RenderFrameGraphResources.h"
#include "Passes/Lighting/Direct/DirectLightingControls.h"
#include "Passes/Lighting/Shadows/DirectShadowControls.h"
#include "Passes/Lighting/Restir/Indirect/IndirectLightingControls.h"

std::uint64_t GetRealTimePathTracerTopologyIdentity(const RenderFrameGraphSettings& settings) noexcept
{
	return (IsDirectLightingAdmitted() ? 1u : 0u) | (IsDirectShadowsActive() ? 2u : 0u) | (IsIndirectLightingAdmitted() ? 4u : 0u)
	    | (HasRequiredRealTimePathTracerProducts(settings) ? 0u : 8u);
}

void AddRealTimePathTracerPasses(
    FrameGraphBuilder& builder,
    const RenderFrameGraphSettings& settings,
    RenderRayTracingScene& rayTracingScene,
    GpuMeshCache& gpuMeshCache,
    RenderFrameGraphResources& resources)
{
	AddGBufferPasses(builder, settings.RenderExtent, gpuMeshCache, rayTracingScene, resources);

	AddRestirLightingPasses(builder, settings.RenderExtent, settings.UseRayReconstruction, rayTracingScene, resources);
	AddLightingCompositePass(builder, settings.RenderExtent, resources);
	AddSkyPass(builder, settings.RenderExtent, resources);

	PublishRealTimePathTracerProducts(resources);
}

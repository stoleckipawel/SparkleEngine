#include "../../../PCH.h"
#include "Passes/Lighting/Restir/RestirDirectLighting.h"

#include "Passes/Lighting/Direct/DirectLightReservoir.h"
#include "Passes/Lighting/Direct/DirectLighting.h"
#include "Passes/Lighting/Shadows/ShadowVisibility.h"
#include "Passes/Lighting/Shadows/DirectShadowSignal.h"

void AddRestirDirectLightingPasses(
    FrameGraphBuilder& builder,
    RenderRayTracingScene& rayTracingScene,
    RenderViewportExtent sceneExtent,
    RenderFrameGraphResources& resources)
{
	const DirectShadowSignalResources shadowSignals = CreateDirectShadowSignalResources(builder, sceneExtent, resources);
	AddDirectLightReservoirPasses(builder, sceneExtent, resources, shadowSignals);
	AddDirectShadowSignalPass(builder, sceneExtent, resources, shadowSignals, rayTracingScene);
	AddDirectLightingPass(builder, sceneExtent, resources, shadowSignals);
}

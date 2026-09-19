#include "../../../PCH.h"
#include "Passes/Lighting/Restir/RestirDirectLightingPasses.h"

#include "Passes/Lighting/Direct/DirectLightReservoirPasses.h"
#include "Passes/Lighting/Direct/DirectLighting.h"
#include "Passes/Lighting/Shadows/DirectShadowSignalResources.h"
#include "Passes/Lighting/Shadows/DirectShadowSignal.h"

void AddRestirDirectLightingPasses(
    FrameGraphBuilder& builder,
    RenderViewportExtent sceneExtent,
    RenderRayTracingScene& rayTracingScene,
    RenderFrameGraphResources& resources)
{
	const DirectShadowSignalResources shadowSignals = CreateDirectShadowSignalResources(builder, sceneExtent, resources);

	AddDirectLightReservoirPasses(builder, sceneExtent, resources, shadowSignals);
	AddDirectShadowSignalPass(builder, sceneExtent, resources, shadowSignals, rayTracingScene);
	AddDirectLightingPass(builder, sceneExtent, resources, shadowSignals);
}

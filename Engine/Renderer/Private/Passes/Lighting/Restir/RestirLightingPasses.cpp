#include "../../../PCH.h"
#include "Passes/Lighting/Restir/RestirLightingPasses.h"

#include "Passes/Lighting/Direct/RestirDirectLightingPasses.h"
#include "Passes/Lighting/Restir/Indirect/RestirIndirectLightingPasses.h"

void AddRestirLightingPasses(
    FrameGraphBuilder& builder,
    RenderViewportExtent sceneExtent,
    bool useRayReconstruction,
    RenderRayTracingScene& rayTracingScene,
    RenderFrameGraphResources& resources)
{
	AddRestirDirectLightingPasses(builder, sceneExtent, rayTracingScene, resources);
	AddRestirIndirectLightingPasses(builder, sceneExtent, useRayReconstruction, resources);
}

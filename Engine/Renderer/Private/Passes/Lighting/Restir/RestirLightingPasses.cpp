#include "../../../PCH.h"
#include "Passes/Lighting/Restir/RestirLightingPasses.h"

#include "Passes/Lighting/Restir/RestirDirectLightingPasses.h"
#include "Passes/Lighting/Restir/RestirIndirectLightingPasses.h"
#include "Passes/Lighting/Restir/RestirLightingResources.h"

void AddRestirLightingPasses(
    FrameGraphBuilder& builder,
    RenderRayTracingScene& rayTracingScene,
    RenderViewportExtent sceneExtent,
    RenderFrameGraphResources& resources)
{
	CreateRestirLightingResources(builder, sceneExtent, resources);
	AddRestirDirectLightingPasses(builder, rayTracingScene, sceneExtent, resources);
	AddRestirIndirectLightingPasses(builder, sceneExtent, resources);
}

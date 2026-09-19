#include "../../../PCH.h"
#include "Passes/Lighting/Restir/RestirLightingPasses.h"

#include "Passes/Lighting/Restir/RestirDirectLightingPasses.h"
#include "Passes/Lighting/Restir/RestirIndirectLightingPasses.h"
#include "Passes/Lighting/Restir/RestirLightingResources.h"

void AddRestirLightingPasses(
    FrameGraphBuilder& builder,
    RenderViewportExtent sceneExtent,
    RenderRayTracingScene& rayTracingScene,
    RenderFrameGraphResources& resources)
{
	CreateRestirLightingResources(builder, sceneExtent, resources);

	AddRestirDirectLightingPasses(builder, sceneExtent, rayTracingScene, resources);
	AddRestirIndirectLightingPasses(builder, sceneExtent, resources);
}

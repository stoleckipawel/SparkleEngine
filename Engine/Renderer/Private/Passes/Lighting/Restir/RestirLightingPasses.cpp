#include "../../../PCH.h"
#include "Passes/Lighting/Restir/RestirLightingPasses.h"

#include "Frame/RenderFrame.h"
#include "Passes/Lighting/Direct/RestirDirectLightingPasses.h"
#include "Passes/Lighting/Restir/Indirect/RestirIndirectLightingPasses.h"

void AddRestirLightingPasses(
    FrameGraphBuilder& builder,
    const RenderFrame& frame,
    RenderViewportExtent sceneExtent,
    RenderRayTracingScene& rayTracingScene,
    RenderFrameGraphResources& resources)
{
	AddRestirDirectLightingPasses(builder, frame, sceneExtent, rayTracingScene, resources);
	AddRestirIndirectLightingPasses(builder, frame, sceneExtent, resources);
}

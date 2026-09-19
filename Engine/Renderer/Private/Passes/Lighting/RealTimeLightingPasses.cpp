#include "../../PCH.h"
#include "Passes/Lighting/RealTimeLightingPasses.h"

#include "Passes/Lighting/LightingComposite.h"
#include "Passes/Lighting/LightingRenderTargets.h"
#include "Passes/Lighting/LightingTargetClear.h"
#include "Passes/Lighting/Restir/RestirLightingPasses.h"
#include "Passes/Lighting/Sky/Sky.h"

void AddRealTimeLightingPasses(
    FrameGraphBuilder& builder,
    RenderRayTracingScene& rayTracingScene,
    RenderViewportExtent sceneExtent,
    RenderFrameGraphResources& resources)
{
	CreateRealTimeLightingRenderTargets(builder, sceneExtent, resources);
	AddLightingTargetClearPass(builder, resources);
	AddRestirLightingPasses(builder, rayTracingScene, sceneExtent, resources);
	AddLightingCompositePass(builder, sceneExtent, resources);
	AddSkyPass(builder, sceneExtent, resources);
}

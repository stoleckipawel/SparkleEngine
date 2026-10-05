#include "../../../PCH.h"
#include "Passes/Lighting/Direct/RestirDirectLightingPasses.h"

#include "Passes/Lighting/Direct/DirectLightReservoirPasses.h"
#include "Passes/Lighting/Direct/DirectLighting.h"
#include "Passes/Lighting/Direct/DirectLightingControls.h"
#include "Passes/Lighting/Direct/DirectLightingResources.h"
#include "Passes/Lighting/Shadows/DirectShadowSignal.h"

void AddRestirDirectLightingPasses(
    FrameGraphBuilder& builder,
    RenderViewportExtent sceneExtent,
    RenderRayTracingScene& rayTracingScene,
    RenderFrameGraphResources& resources)
{
	CreateDirectLightingResources(builder, sceneExtent, resources);
	if (!IsDirectLightingAdmitted())
	{
		return;
	}
	CreateDirectLightReservoirResources(builder, sceneExtent, resources);

	AddDirectLightReservoirPasses(builder, sceneExtent, resources);
	AddDirectShadowSignalPass(builder, sceneExtent, resources, rayTracingScene);
	AddDirectLightingPass(builder, sceneExtent, resources);
}

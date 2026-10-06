#include "../../../PCH.h"
#include "Passes/Lighting/Direct/RestirDirectLightingPasses.h"

#include "Frame/RenderFrame.h"
#include "Passes/Lighting/Direct/DirectLightReservoirPasses.h"
#include "Passes/Lighting/Direct/DirectLighting.h"
#include "Passes/Lighting/Direct/DirectLightingControls.h"
#include "Passes/Lighting/Direct/DirectLightingResources.h"
#include "Passes/Lighting/Shadows/DirectShadowSignal.h"

void AddRestirDirectLightingPasses(
    FrameGraphBuilder& builder,
    const RenderFrame& frame,
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

	AddDirectLightReservoirPasses(builder, frame, sceneExtent, resources);
	AddDirectShadowSignalPass(builder, frame, sceneExtent, resources, rayTracingScene);
	AddDirectLightingPass(builder, frame, sceneExtent, resources);
}

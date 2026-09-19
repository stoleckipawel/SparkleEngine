#include "../../../PCH.h"
#include "Passes/Lighting/Direct/DirectLightReservoirPasses.h"

#include "Passes/Lighting/Direct/DirectLightReservoirPassDefinitions.h"

void AddDirectLightReservoirPasses(
    FrameGraphBuilder& builder,
    RenderViewportExtent sceneExtent,
    const RenderFrameGraphResources& resources,
    const DirectShadowSignalResources& shadowSignals)
{
	AddDirectLightReservoirTemporalPass(builder, sceneExtent, resources, shadowSignals);
	AddDirectLightReservoirSpatialPass(builder, sceneExtent, resources, shadowSignals);
}

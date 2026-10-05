#include "../../../PCH.h"
#include "Passes/Lighting/Direct/DirectLightReservoirPasses.h"

#include "Passes/Lighting/Direct/DirectLightReservoirPassDefinitions.h"

void AddDirectLightReservoirPasses(FrameGraphBuilder& builder, RenderViewportExtent sceneExtent, const RenderFrameGraphResources& resources)
{
	AddDirectLightReservoirTemporalPass(builder, sceneExtent, resources);
	AddDirectLightReservoirSpatialPass(builder, sceneExtent, resources);
}

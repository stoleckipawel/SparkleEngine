#include "../../../PCH.h"
#include "Passes/Lighting/Direct/DirectLightReservoirPasses.h"

#include "Frame/RenderFrame.h"
#include "Passes/Lighting/Direct/DirectLightReservoirPassDefinitions.h"

void AddDirectLightReservoirPasses(FrameGraphBuilder& builder, const RenderFrame& frame, RenderViewportExtent sceneExtent, const RenderFrameGraphResources& resources)
{
	AddDirectLightReservoirTemporalPass(builder, frame, sceneExtent, resources);
	AddDirectLightReservoirSpatialPass(builder, frame, sceneExtent, resources);
}

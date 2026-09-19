#include "../../../PCH.h"
#include "Passes/Lighting/Restir/RestirIndirectLightingPasses.h"

#include "Passes/Lighting/Restir/RestirIndirectReservoirPasses.h"
#include "Passes/Lighting/Restir/RestirIndirectResolve.h"
#include "Passes/Lighting/Restir/RestirIndirectSpatial.h"
#include "Passes/Lighting/Restir/RestirIndirectTemporal.h"

void AddRestirIndirectLightingPasses(FrameGraphBuilder& builder, RenderViewportExtent sceneExtent, RenderFrameGraphResources& resources)
{
	const RestirIndirectWorkingReservoirs workingReservoirs =
	    AddRestirIndirectReservoirPasses(builder, sceneExtent, resources);
	AddRestirIndirectTemporalPass(builder, sceneExtent, workingReservoirs, resources);
	AddRestirIndirectSpatialPass(builder, sceneExtent, workingReservoirs, resources);
	AddRestirIndirectResolvePass(builder, sceneExtent, resources);
}

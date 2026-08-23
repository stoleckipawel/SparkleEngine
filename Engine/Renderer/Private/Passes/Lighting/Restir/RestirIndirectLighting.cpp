#include "../../../PCH.h"
#include "Passes/Lighting/Restir/RestirIndirectLighting.h"

#include "Passes/Lighting/Restir/RestirIndirectReservoirs.h"
#include "Passes/Lighting/Restir/RestirIndirectResolve.h"
#include "Passes/Lighting/Restir/RestirIndirectSpatial.h"
#include "Passes/Lighting/Restir/RestirIndirectTemporal.h"

void AddRestirIndirectLightingPasses(FrameGraphBuilder& builder, RenderViewportExtent sceneExtent, RenderFrameGraphResources& resources)
{
	const RestirIndirectWorkingReservoirs workingReservoirs = CreateRestirIndirectWorkingReservoirs(builder, sceneExtent);
	AddRestirIndirectReservoirClearPasses(builder, sceneExtent, workingReservoirs, resources.History.RestirIndirectReservoir);
	AddRestirIndirectTemporalPass(builder, sceneExtent, workingReservoirs, resources);
	AddRestirIndirectSpatialPass(builder, sceneExtent, workingReservoirs, resources);
	AddRestirIndirectResolvePass(builder, sceneExtent, resources);
}

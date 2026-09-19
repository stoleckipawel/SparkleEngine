#include "PCH.h"
#include "Passes/Lighting/Restir/Indirect/RestirIndirectLightingPasses.h"

#include "Passes/Lighting/Restir/Indirect/RestirIndirectReservoirPasses.h"
#include "Passes/Lighting/Restir/Indirect/RestirIndirectResolve.h"
#include "Passes/Lighting/Restir/Indirect/RestirIndirectSpatial.h"
#include "Passes/Lighting/Restir/Indirect/RestirIndirectTemporal.h"

void AddRestirIndirectLightingPasses(FrameGraphBuilder& builder, RenderViewportExtent sceneExtent, RenderFrameGraphResources& resources)
{
	const RestirIndirectWorkingReservoirs workingReservoirs = AddRestirIndirectReservoirPasses(builder, sceneExtent, resources);

	AddRestirIndirectTemporalPass(builder, sceneExtent, workingReservoirs, resources);
	AddRestirIndirectSpatialPass(builder, sceneExtent, workingReservoirs, resources);
	AddRestirIndirectResolvePass(builder, sceneExtent, resources);
}

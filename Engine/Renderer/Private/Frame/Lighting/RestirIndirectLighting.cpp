#include "../../PCH.h"
#include "Frame/Lighting/RestirIndirectLighting.h"

#include "Frame/Lighting/RestirIndirectReservoirs.h"
#include "Frame/Lighting/RestirIndirectResolve.h"
#include "Frame/Lighting/RestirIndirectSpatial.h"
#include "Frame/Lighting/RestirIndirectTemporal.h"

void AddRestirIndirectLightingPasses(FrameGraphBuilder& builder, RenderViewportExtent sceneExtent, FrameAssemblyResourceLayout& resources)
{
	const RestirIndirectWorkingReservoirs workingReservoirs = CreateRestirIndirectWorkingReservoirs(builder, sceneExtent);
	AddRestirIndirectReservoirClearPasses(builder, sceneExtent, workingReservoirs, resources.History.RestirIndirectReservoir);
	AddRestirIndirectTemporalPass(builder, sceneExtent, workingReservoirs, resources);
	AddRestirIndirectSpatialPass(builder, sceneExtent, workingReservoirs, resources);
	AddRestirIndirectResolvePass(builder, sceneExtent, resources);
}

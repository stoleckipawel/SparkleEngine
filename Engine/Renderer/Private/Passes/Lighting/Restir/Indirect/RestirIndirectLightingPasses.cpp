#include "PCH.h"
#include "Passes/Lighting/Restir/Indirect/RestirIndirectLightingPasses.h"

#include "Passes/Lighting/Restir/Indirect/RestirIndirectReservoirPasses.h"
#include "Passes/Lighting/Restir/Indirect/RestirIndirectResolve.h"
#include "Passes/Lighting/Restir/Indirect/RestirIndirectSpatial.h"
#include "Passes/Lighting/Restir/Indirect/RestirIndirectTemporal.h"
#include "Passes/Lighting/Restir/Indirect/RestirIndirectReservoirResources.h"
#include "Passes/Lighting/Restir/Indirect/IndirectLightingControls.h"
#include "Passes/Lighting/Restir/Indirect/IndirectLightingResources.h"

void AddRestirIndirectLightingPasses(
    FrameGraphBuilder& builder,
    RenderViewportExtent sceneExtent,
    bool useRayReconstruction,
    RenderFrameGraphResources& resources)
{
	const bool admitted = IsIndirectLightingAdmitted();
	CreateIndirectLightingResources(builder, sceneExtent, admitted, resources);
	if (!admitted)
	{
		return;
	}
	CreateRestirIndirectHistoryResources(builder, sceneExtent, resources);
	const RestirIndirectWorkingReservoirs workingReservoirs = AddRestirIndirectReservoirPasses(builder, sceneExtent, resources);

	AddRestirIndirectTemporalPass(builder, sceneExtent, workingReservoirs, resources);
	AddRestirIndirectSpatialPass(builder, sceneExtent, workingReservoirs, resources);
	AddRestirIndirectResolvePass(builder, sceneExtent, useRayReconstruction, resources);
}

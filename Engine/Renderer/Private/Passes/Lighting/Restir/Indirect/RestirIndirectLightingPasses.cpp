#include "PCH.h"
#include "Passes/Lighting/Restir/Indirect/RestirIndirectLightingPasses.h"

#include "Frame/RenderFrame.h"
#include "Passes/Lighting/Restir/Indirect/RestirIndirectReservoirPasses.h"
#include "Passes/Lighting/Restir/Indirect/RestirIndirectResolve.h"
#include "Passes/Lighting/Restir/Indirect/RestirIndirectSpatial.h"
#include "Passes/Lighting/Restir/Indirect/RestirIndirectTemporal.h"
#include "Passes/Lighting/Restir/Indirect/RestirIndirectReservoirResources.h"
#include "Passes/Lighting/Restir/Indirect/IndirectLightingControls.h"
#include "Passes/Lighting/Restir/Indirect/IndirectLightingResources.h"
#include "Passes/Lighting/Restir/Reconstruction/RayReconstructionSurfaceGuides.h"

void AddRestirIndirectLightingPasses(
    FrameGraphBuilder& builder,
    const RenderFrame& frame,
    RenderViewportExtent sceneExtent,
    RenderFrameGraphResources& resources)
{
	CreateIndirectLightingResources(builder, sceneExtent, resources);
	AddRayReconstructionSurfaceGuidesPass(builder, frame, sceneExtent, resources);
	if (!IsIndirectLightingAdmitted())
	{
		return;
	}
	CreateRestirIndirectHistoryResources(builder, sceneExtent, resources);
	const RestirIndirectWorkingReservoirs workingReservoirs = AddRestirIndirectReservoirPasses(builder, sceneExtent, resources);

	AddRestirIndirectTemporalPass(builder, frame, sceneExtent, workingReservoirs, resources);
	AddRestirIndirectSpatialPass(builder, frame, sceneExtent, workingReservoirs, resources);
	AddRestirIndirectResolvePass(builder, frame, sceneExtent, resources);
}

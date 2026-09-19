#include "../../../PCH.h"
#include "Passes/Lighting/Restir/RestirIndirectReservoirPasses.h"

#include "Frame/Graph/RenderFrameGraphResources.h"
#include "Passes/Lighting/Restir/RestirIndirectReservoirResources.h"
#include "Passes/Utility/ComputeClear.h"
#include "Resources/History/FrameHistory.h"

RestirIndirectWorkingReservoirs AddRestirIndirectReservoirPasses(
    FrameGraphBuilder& builder,
    RenderViewportExtent sceneExtent,
    const RenderFrameGraphResources& resources)
{
	const RestirIndirectWorkingReservoirs workingReservoirs = CreateRestirIndirectWorkingReservoirs(builder, sceneExtent);
	const FrameGraphReservoirHistoryHandles& history = resources.History.RestirIndirectReservoir;
	AddComputeClearPass(builder, "ClearRestirIndirectTemporalSample", workingReservoirs.TemporalSample, sceneExtent);
	AddComputeClearPass(builder, "ClearRestirIndirectTemporalWeight", workingReservoirs.TemporalWeight, sceneExtent);
	AddComputeClearPass(builder, "ClearRestirIndirectCurrentSample", history.Sample.Current, sceneExtent);
	AddComputeClearPass(builder, "ClearRestirIndirectCurrentWeight", history.Weight.Current, sceneExtent);
	AddComputeClearPass(builder, "ClearRestirIndirectCurrentSurface", history.Surface.Current, sceneExtent);
	return workingReservoirs;
}

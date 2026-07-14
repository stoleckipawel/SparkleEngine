#include "../../PCH.h"
#include "Frame/Lighting/RestirIndirectReservoirs.h"

#include "FrameGraph/Builder/FrameGraphBuilder.h"
#include "Passes/Utility/ComputeClearPass.h"
#include "Renderer/Public/FrameGraph/FrameGraphTextureDesc.h"
#include "RHI/Public/Formats/PixelFormat.h"

RestirIndirectWorkingReservoirs CreateRestirIndirectWorkingReservoirs(FrameGraphBuilder& builder, RenderViewportExtent sceneExtent)
{
	const auto createReservoirTexture = [&](const char* name)
	{
		return builder.CreateTexture(
		    FrameGraphTextureDesc::CreateColor(name, sceneExtent.Width, sceneExtent.Height, PixelFormat::R32G32B32A32_Float));
	};
	return RestirIndirectWorkingReservoirs{
	    .TemporalSample = createReservoirTexture("RestirIndirectTemporalReservoirSample"),
	    .TemporalWeight = createReservoirTexture("RestirIndirectTemporalReservoirWeight")};
}

void AddRestirIndirectReservoirClearPasses(
    FrameGraphBuilder& builder,
    RenderViewportExtent sceneExtent,
    const RestirIndirectWorkingReservoirs& workingReservoirs,
    const FrameGraphReservoirHistoryHandles& history)
{
	AddComputeClearPass(builder, "ClearRestirIndirectTemporalSample", workingReservoirs.TemporalSample, sceneExtent);
	AddComputeClearPass(builder, "ClearRestirIndirectTemporalWeight", workingReservoirs.TemporalWeight, sceneExtent);
	AddComputeClearPass(builder, "ClearRestirIndirectCurrentSample", history.Sample.Current, sceneExtent);
	AddComputeClearPass(builder, "ClearRestirIndirectCurrentWeight", history.Weight.Current, sceneExtent);
	AddComputeClearPass(builder, "ClearRestirIndirectCurrentSurface", history.Surface.Current, sceneExtent);
}

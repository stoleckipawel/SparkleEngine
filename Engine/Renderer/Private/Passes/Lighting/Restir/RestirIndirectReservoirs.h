#pragma once

#include "Resources/History/FrameHistory.h"
#include "Renderer/Public/FrameGraph/FrameGraphTextureHandle.h"
#include "Renderer/Public/Viewport/ViewportContracts.h"

class FrameGraphBuilder;

struct RestirIndirectWorkingReservoirs final
{
	FrameGraphTextureHandle TemporalSample = FrameGraphTextureHandle::Invalid();
	FrameGraphTextureHandle TemporalWeight = FrameGraphTextureHandle::Invalid();
};

RestirIndirectWorkingReservoirs CreateRestirIndirectWorkingReservoirs(FrameGraphBuilder& builder, RenderViewportExtent sceneExtent);

void AddRestirIndirectReservoirClearPasses(
    FrameGraphBuilder& builder,
    RenderViewportExtent sceneExtent,
    const RestirIndirectWorkingReservoirs& workingReservoirs,
    const FrameGraphReservoirHistoryHandles& history);

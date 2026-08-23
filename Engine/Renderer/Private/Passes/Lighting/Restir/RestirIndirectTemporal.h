#pragma once

#include "Frame/Graph/RenderFrameGraphResources.h"
#include "Passes/Lighting/Restir/RestirIndirectReservoirs.h"

class FrameGraphBuilder;

void AddRestirIndirectTemporalPass(
    FrameGraphBuilder& builder,
    RenderViewportExtent sceneExtent,
    const RestirIndirectWorkingReservoirs& workingReservoirs,
    const RenderFrameGraphResources& resources);

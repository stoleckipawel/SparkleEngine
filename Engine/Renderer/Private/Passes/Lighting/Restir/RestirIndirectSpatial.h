#pragma once

#include "Frame/Graph/RenderFrameGraphResources.h"
#include "Passes/Lighting/Restir/RestirIndirectReservoirs.h"

class FrameGraphBuilder;

void AddRestirIndirectSpatialPass(
    FrameGraphBuilder& builder,
    RenderViewportExtent sceneExtent,
    const RestirIndirectWorkingReservoirs& workingReservoirs,
    const RenderFrameGraphResources& resources);

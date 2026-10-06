#pragma once

struct RenderFrame;

#include "Frame/Graph/RenderFrameGraphResources.h"
#include "Passes/Lighting/Restir/Indirect/RestirIndirectReservoirResources.h"

class FrameGraphBuilder;

void AddRestirIndirectSpatialPass(
    FrameGraphBuilder& builder,
    const RenderFrame& frame,
    RenderViewportExtent sceneExtent,
    const RestirIndirectWorkingReservoirs& workingReservoirs,
    const RenderFrameGraphResources& resources);

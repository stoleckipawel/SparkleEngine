#pragma once

#include "Frame/Core/FrameAssembly.h"
#include "Frame/Lighting/RestirIndirectReservoirs.h"

class FrameGraphBuilder;

void AddRestirIndirectSpatialPass(
    FrameGraphBuilder& builder,
    RenderViewportExtent sceneExtent,
    const RestirIndirectWorkingReservoirs& workingReservoirs,
    const FrameAssemblyResourceLayout& resources);

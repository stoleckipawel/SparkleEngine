#pragma once

#include "Passes/Lighting/Restir/RestirIndirectReservoirResources.h"
#include "Renderer/Public/Viewport/ViewportContracts.h"

class FrameGraphBuilder;
struct RenderFrameGraphResources;

RestirIndirectWorkingReservoirs AddRestirIndirectReservoirPasses(
    FrameGraphBuilder& builder,
    RenderViewportExtent sceneExtent,
    const RenderFrameGraphResources& resources);

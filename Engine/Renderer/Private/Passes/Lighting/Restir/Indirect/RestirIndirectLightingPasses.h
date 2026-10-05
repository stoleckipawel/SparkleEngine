#pragma once

#include "Frame/Graph/RenderFrameGraphResources.h"
#include "Renderer/Public/Viewport/ViewportContracts.h"

class FrameGraphBuilder;

void AddRestirIndirectLightingPasses(
    FrameGraphBuilder& builder,
    RenderViewportExtent sceneExtent,
    bool useRayReconstruction,
    RenderFrameGraphResources& resources);

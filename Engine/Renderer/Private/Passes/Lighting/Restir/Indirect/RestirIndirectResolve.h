#pragma once

#include "Frame/Graph/RenderFrameGraphResources.h"

class FrameGraphBuilder;

void AddRestirIndirectResolvePass(
    FrameGraphBuilder& builder,
    RenderViewportExtent sceneExtent,
    bool writeRayReconstructionGuides,
    const RenderFrameGraphResources& resources);

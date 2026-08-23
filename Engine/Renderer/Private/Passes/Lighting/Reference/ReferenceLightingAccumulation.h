#pragma once

#include "Frame/Graph/RenderFrameGraphResources.h"

class FrameGraphBuilder;

void AddReferenceLightingAccumulationPass(
    FrameGraphBuilder& builder,
    RenderViewportExtent sceneExtent,
    FrameGraphTextureHandle referenceSample,
    const RenderFrameGraphResources& resources);

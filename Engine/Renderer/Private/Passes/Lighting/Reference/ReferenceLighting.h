#pragma once

#include "Frame/Graph/RenderFrameGraphResources.h"

class FrameGraphBuilder;

void AddReferenceLightingProducerPasses(
    FrameGraphBuilder& builder,
    RenderViewportExtent sceneExtent,
    const RenderFrameGraphResources& resources);
void FinalizeReferenceLightingPasses(
    FrameGraphBuilder& builder,
    RenderViewportExtent sceneExtent,
    FrameGraphTextureHandle referenceSample,
    const RenderFrameGraphResources& resources);

#pragma once

#include "Frame/Graph/RenderFrameGraphResources.h"

class FrameGraphBuilder;

void AddReferencePathTracerAccumulationPass(
    FrameGraphBuilder& builder,
    RenderViewportExtent sceneExtent,
    FrameGraphTextureHandle referencePathTracerSample,
    const RenderFrameGraphResources& resources);

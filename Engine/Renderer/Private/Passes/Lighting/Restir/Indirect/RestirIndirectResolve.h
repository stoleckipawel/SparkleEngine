#pragma once

struct RenderFrame;

#include "Frame/Graph/RenderFrameGraphResources.h"

class FrameGraphBuilder;

void AddRestirIndirectResolvePass(
    FrameGraphBuilder& builder,
    const RenderFrame& frame,
    RenderViewportExtent sceneExtent,
    const RenderFrameGraphResources& resources);

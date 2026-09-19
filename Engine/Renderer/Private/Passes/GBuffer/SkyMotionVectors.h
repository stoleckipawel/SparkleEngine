#pragma once

#include "Renderer/Public/Viewport/ViewportContracts.h"

class FrameGraphBuilder;
struct RenderFrameGraphResources;

void AddSkyMotionVectorPass(
    FrameGraphBuilder& builder,
    RenderViewportExtent sceneExtent,
    const RenderFrameGraphResources& resources);

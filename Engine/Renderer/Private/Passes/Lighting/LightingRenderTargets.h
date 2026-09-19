#pragma once

#include "Renderer/Public/Viewport/ViewportContracts.h"

class FrameGraphBuilder;
struct RenderFrameGraphResources;

void CreateRealTimeLightingRenderTargets(
    FrameGraphBuilder& builder,
    RenderViewportExtent sceneExtent,
    RenderFrameGraphResources& resources);

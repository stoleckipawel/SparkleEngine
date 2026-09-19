#pragma once

#include "Renderer/Public/Viewport/RenderViewMode.h"
#include "Renderer/Public/Viewport/ViewportContracts.h"

class FrameGraphBuilder;
struct RenderFrameGraphResources;

void AddLightingVisualizationPass(
    FrameGraphBuilder& builder,
    RenderViewportExtent sceneExtent,
    RenderViewMode viewMode,
    const RenderFrameGraphResources& resources);

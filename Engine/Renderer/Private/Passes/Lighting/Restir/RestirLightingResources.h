#pragma once

#include "Renderer/Public/Viewport/ViewportContracts.h"

class FrameGraphBuilder;
struct RenderFrameGraphResources;

void CreateRestirLightingResources(
    FrameGraphBuilder& builder,
    RenderViewportExtent sceneExtent,
    RenderFrameGraphResources& resources);

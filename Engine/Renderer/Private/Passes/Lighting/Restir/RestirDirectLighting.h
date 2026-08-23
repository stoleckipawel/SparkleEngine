#pragma once

#include "Frame/Graph/RenderFrameGraphResources.h"
#include "Renderer/Public/Viewport/ViewportContracts.h"

class FrameGraphBuilder;

void AddRestirDirectLightingPasses(
    FrameGraphBuilder& builder,
    bool enableInlineRayQueryShadows,
    RenderViewportExtent sceneExtent,
    RenderFrameGraphResources& resources);

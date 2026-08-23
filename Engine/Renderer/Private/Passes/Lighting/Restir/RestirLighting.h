#pragma once

#include "Frame/Graph/RenderFrameGraphResources.h"
#include "Renderer/Public/Viewport/ViewportContracts.h"

class FrameGraphBuilder;

void AddRestirLightingProducerPasses(
    FrameGraphBuilder& builder,
    bool enableInlineRayQueryShadows,
    RenderViewportExtent sceneExtent,
    RenderFrameGraphResources& resources);

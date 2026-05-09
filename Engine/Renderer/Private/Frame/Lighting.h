#pragma once

#include "FrameGraph/Features/FrameGraphProducts.h"
#include "Renderer/Public/Viewport/ViewportContracts.h"

class FrameGraph;

void BuildLighting(
    FrameGraph& frameGraph,
    RenderViewportExtent sceneExtent,
    const SceneTargets& sceneTargets,
    const GBufferTargets& gbuffer);

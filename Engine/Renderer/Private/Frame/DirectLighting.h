#pragma once

#include "FrameGraph/Features/FrameGraphProducts.h"

class FrameGraph;

void BuildDirectLighting(
    FrameGraph& frameGraph,
    const SceneTargets& sceneTargets,
    const GBufferTargets& gbuffer);

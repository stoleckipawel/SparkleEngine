#pragma once

#include "FrameGraph/Features/FrameGraphProducts.h"

class FrameGraph;

void BuildLighting(
    FrameGraph& frameGraph,
    const SceneTargets& sceneTargets,
    const GBufferTargets& gbuffer);

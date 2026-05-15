#pragma once

#include "FrameGraph/Features/FrameGraphProducts.h"

class FrameGraph;

void AddVisualizeBuffersPass(
    FrameGraph& frameGraph,
    const SceneTargets& sceneTargets,
    const LightingTargets& lighting,
    const GBufferTargets& gbuffer);
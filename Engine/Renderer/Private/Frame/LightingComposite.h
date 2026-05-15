#pragma once

#include "FrameGraph/Features/FrameGraphProducts.h"

class FrameGraph;

void AddLightingCompositePass(
    FrameGraph& frameGraph,
    const SceneTargets& sceneTargets,
    const LightingTargets& lighting,
    const GBufferTargets& gbuffer);
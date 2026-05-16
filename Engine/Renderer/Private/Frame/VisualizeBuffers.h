#pragma once

#include "FrameGraph/Features/FrameGraphProducts.h"

class FrameGraphBuilder;

void AddVisualizeBuffersPass(
    FrameGraphBuilder& builder,
    const SceneTargets& sceneTargets,
    const LightingTargets& lighting,
    const GBufferTargets& gbuffer);

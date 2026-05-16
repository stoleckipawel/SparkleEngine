#pragma once

#include "FrameGraph/Features/FrameGraphProducts.h"

class FrameGraphBuilder;

void AddLightingCompositePass(
    FrameGraphBuilder& builder,
    const SceneTargets& sceneTargets,
    const LightingTargets& lighting,
    const GBufferTargets& gbuffer);

#pragma once

#include "Frame/Targets/FrameRenderTargets.h"

class FrameGraphBuilder;
struct DirectShadowSignalResources;

void AddDirectLightingPass(
    FrameGraphBuilder& builder,
    const LightingRenderTargets& lighting,
    const SceneRenderTargets& sceneTargets,
    const GBufferRenderTargets& gbuffer,
    const DirectShadowSignalResources& shadowSignals);

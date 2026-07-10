#pragma once

#include "Frame/Targets/FrameRenderTargets.h"

class FrameGraphBuilder;

void AddLightingCompositePass(
    FrameGraphBuilder& builder,
    FrameGraphTextureHandle output,
    const LightingRenderTargets& lighting,
    const GBufferRenderTargets& gbuffer);

#pragma once

#include "Frame/Targets/FrameRenderTargets.h"

class FrameGraphBuilder;
struct DirectShadowSignalResources;

void AddDirectLightReservoirPasses(
    FrameGraphBuilder& builder,
    const GBufferRenderTargets& gbuffer,
    const DirectShadowSignalResources& shadowSignals);

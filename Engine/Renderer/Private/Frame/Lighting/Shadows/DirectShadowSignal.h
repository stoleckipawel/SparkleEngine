#pragma once

#include "Frame/Targets/FrameRenderTargets.h"
#include "Renderer/Public/FrameGraph/FrameGraphAccelerationStructureHandle.h"
#include "Renderer/Public/FrameGraph/FrameGraphTextureHandle.h"

class FrameGraphBuilder;

void AddDirectShadowSignalPass(
    FrameGraphBuilder& builder,
    const GBufferRenderTargets& gbuffer,
    FrameGraphAccelerationStructureHandle sceneTlas,
    FrameGraphTextureHandle shadowVisibilitySignal);

#pragma once

#include "Frame/Graph/RenderFrameGraphTargets.h"
#include "Renderer/Public/Viewport/ViewportContracts.h"

class FrameGraphBuilder;

void AddVisualizeBuffersPass(
    FrameGraphBuilder& builder,
    RenderViewportExtent sceneExtent,
    FrameGraphTextureHandle resolvedSceneColor,
    const LightingRenderTargets& lighting,
    const GBufferRenderTargets& gbuffer);

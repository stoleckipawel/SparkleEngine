#pragma once

#include "Frame/Targets/FrameRenderTargets.h"
#include "Renderer/Public/FrameGraph/FrameGraphTextureHandle.h"
#include "Renderer/Public/Viewport/ViewportContracts.h"
#include "RHI/Public/Formats/PixelFormat.h"

class FrameGraphBuilder;

void AddPresentationPass(
    FrameGraphBuilder& builder,
    RenderViewportExtent sceneExtent,
    PixelFormat backBufferFormat,
    const SceneRenderTargets& sceneTargets,
    FrameGraphTextureHandle exposure);

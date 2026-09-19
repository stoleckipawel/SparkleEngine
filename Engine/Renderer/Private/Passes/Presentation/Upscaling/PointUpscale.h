#pragma once

#include "FrameGraph/FrameGraphTextureHandle.h"
#include "Renderer/Public/Viewport/ViewportContracts.h"

class FrameGraphBuilder;

void AddPointUpscalePass(
    FrameGraphBuilder& builder,
    FrameGraphTextureHandle inputColor,
    FrameGraphTextureHandle outputColor,
    RenderViewportExtent outputExtent);

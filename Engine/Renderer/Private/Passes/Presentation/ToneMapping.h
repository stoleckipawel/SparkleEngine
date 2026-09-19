#pragma once

#include "FrameGraph/FrameGraphTextureHandle.h"
#include "Renderer/Public/Viewport/ViewportContracts.h"

class FrameGraphBuilder;
struct RenderFrameGraphResources;

FrameGraphTextureHandle AddToneMappingPass(
    FrameGraphBuilder& builder,
    RenderViewportExtent outputExtent,
    const RenderFrameGraphResources& resources);

#pragma once

#include "FrameGraph/FrameGraphTextureHandle.h"
#include "Renderer/Public/Viewport/RenderViewMode.h"
#include "Renderer/Public/Viewport/ViewportContracts.h"

class FrameGraphBuilder;
struct RenderFrameGraphResources;

FrameGraphTextureHandle AddDisplayMappingPass(
    FrameGraphBuilder& builder,
    RenderViewportExtent outputExtent,
    RenderViewMode viewMode,
    const RenderFrameGraphResources& resources);

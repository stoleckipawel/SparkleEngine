#pragma once

#include "Frame/Graph/RenderFrameGraphResources.h"
#include "Renderer/Public/Viewport/ViewportContracts.h"

class FrameGraphBuilder;
class IUpscalerProvider;

void AddUpscalingPasses(
    FrameGraphBuilder& builder,
    RenderViewportExtent renderExtent,
    RenderViewportExtent outputExtent,
    IUpscalerProvider* upscalerProvider,
    RenderFrameGraphResources& resources);

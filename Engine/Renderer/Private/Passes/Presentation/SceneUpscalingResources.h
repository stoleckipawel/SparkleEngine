#pragma once

#include "Upscaling/UpscalerPass.h"
#include "Renderer/Public/Viewport/ViewportContracts.h"

class FrameGraphBuilder;
struct RenderFrameGraphResources;

UpscalerPassResources CreateSceneUpscalingResources(
    FrameGraphBuilder& builder,
    RenderViewportExtent outputExtent,
    RenderFrameGraphResources& resources);

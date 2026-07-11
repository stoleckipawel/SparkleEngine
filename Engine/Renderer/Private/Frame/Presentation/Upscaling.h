#pragma once

#include "Frame/Core/FrameAssembly.h"
#include "Renderer/Public/Viewport/ViewportContracts.h"

class FrameGraphBuilder;

void AddUpscalingPasses(
    FrameGraphBuilder& builder,
    RenderViewportExtent renderExtent,
    RenderViewportExtent outputExtent,
    FrameAssemblyResourceLayout& resources);

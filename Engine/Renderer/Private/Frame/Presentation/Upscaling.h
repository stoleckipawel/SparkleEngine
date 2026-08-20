#pragma once

#include "Frame/Core/FrameAssembly.h"
#include "Renderer/Public/Viewport/ViewportContracts.h"

class FrameGraphBuilder;
class IUpscalerProvider;

void AddUpscalingPasses(
    FrameGraphBuilder& builder,
    RenderViewportExtent renderExtent,
    RenderViewportExtent outputExtent,
    IUpscalerProvider* upscalerProvider,
    FrameAssemblyResourceLayout& resources);

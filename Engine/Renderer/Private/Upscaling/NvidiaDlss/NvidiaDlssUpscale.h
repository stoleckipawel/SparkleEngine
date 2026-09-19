#pragma once

#include "Upscaling/UpscalerPass.h"

class FrameGraphBuilder;
class RendererImageProviderStack;

void AddNvidiaDlssUpscalePass(
    FrameGraphBuilder& builder,
    RenderViewportExtent renderExtent,
    RenderViewportExtent outputExtent,
    RendererImageProviderStack& imageProviders,
    const UpscalerPassResources& inputs);

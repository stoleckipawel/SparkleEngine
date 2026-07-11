#pragma once

#include "Frame/Core/FrameAssembly.h"
#include "Frame/Targets/FrameRenderTargets.h"
#include "Renderer/Public/Viewport/ViewportContracts.h"
#include "RHI/Public/Formats/PixelFormat.h"

class FrameGraphBuilder;

void AddPreReconstructionPostProcessingPasses(
    FrameGraphBuilder& builder,
    RenderViewportExtent renderExtent,
    FrameAssemblyResourceLayout& resources);

void AddPostProcessingPasses(
    FrameGraphBuilder& builder,
    RenderViewportExtent renderExtent,
    RenderViewportExtent outputExtent,
    PixelFormat backBufferFormat,
    bool presentToBackBuffer,
    FrameAssemblyResourceLayout& resources);

#pragma once

#include "Frame/Core/FrameAssembly.h"
#include "Frame/Core/FrameRenderPath.h"
#include "RHI/Public/Formats/PixelFormat.h"
#include "Renderer/Public/Viewport/ViewportContracts.h"

class FrameGraphBuilder;

struct FrameBuildResult
{
	FrameAssemblyResourceLayout Resources = {};
	FrameRenderPath RenderPath = FrameRenderPath::RealtimeDeferred;
};

FrameBuildResult BuildFrame(
    FrameGraphBuilder& builder,
    RenderViewportExtent renderExtent,
    RenderViewportExtent outputExtent,
    PixelFormat backBufferFormat,
    bool presentToBackBuffer);

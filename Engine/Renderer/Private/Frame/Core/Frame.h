#pragma once

#include "Frame/Core/FrameAssembly.h"
#include "RHI/Public/Formats/PixelFormat.h"
#include "Renderer/Public/Viewport/ViewportContracts.h"

class FrameGraphBuilder;

struct FrameBuildResult
{
	FrameAssemblyResourceLayout Resources = {};
};

FrameBuildResult BuildFrame(
    FrameGraphBuilder& builder,
    RenderViewportExtent sceneExtent,
    PixelFormat backBufferFormat,
    bool presentToBackBuffer);

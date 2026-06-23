#pragma once

#include "Frame/Core/FrameAssembly.h"
#include "RHI/Public/Formats/PixelFormat.h"
#include "Renderer/Public/Viewport/ViewportContracts.h"

class FrameGraphBuilder;

void CreateFrameSceneResources(
    FrameGraphBuilder& builder,
    RenderViewportExtent sceneExtent,
    PixelFormat backBufferFormat,
    FrameAssemblyResourceLayout& resources);

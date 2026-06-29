#pragma once

#include "Renderer/Public/FrameGraph/FrameGraphTextureHandle.h"
#include "Renderer/Public/Viewport/ViewportContracts.h"

class FrameGraphBuilder;

void AddExposurePass(
    FrameGraphBuilder& builder,
    RenderViewportExtent sceneExtent,
    FrameGraphTextureHandle finalSceneColor,
    FrameGraphTextureHandle previousExposure,
    FrameGraphTextureHandle currentExposure,
    FrameGraphTextureHandle exposure);

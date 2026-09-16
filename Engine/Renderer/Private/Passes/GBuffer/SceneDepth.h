#pragma once

#include "FrameGraph/FrameGraphTextureHandle.h"
#include "Renderer/Public/Viewport/ViewportContracts.h"

class FrameGraphBuilder;

void AddLinearizeDeviceZPass(
    FrameGraphBuilder& builder,
    RenderViewportExtent sceneExtent,
    FrameGraphTextureHandle deviceZ,
    FrameGraphTextureHandle sceneDepth);

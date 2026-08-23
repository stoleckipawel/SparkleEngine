#pragma once

#include "Renderer/Public/FrameGraph/FrameGraphTextureHandle.h"
#include "Renderer/Public/Viewport/ViewportContracts.h"

class FrameGraphBuilder;

void AddLinearizeDeviceZPass(
    FrameGraphBuilder& builder,
    RenderViewportExtent sceneExtent,
    FrameGraphTextureHandle deviceZ,
    FrameGraphTextureHandle sceneDepth);

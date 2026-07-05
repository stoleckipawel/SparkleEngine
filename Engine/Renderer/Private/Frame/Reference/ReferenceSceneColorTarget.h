#pragma once

#include "Renderer/Public/FrameGraph/FrameGraphTextureHandle.h"
#include "Renderer/Public/Viewport/ViewportContracts.h"

class FrameGraphBuilder;

FrameGraphTextureHandle CreateReferenceSceneColorTarget(FrameGraphBuilder& builder, RenderViewportExtent sceneExtent);
void AddReferenceSceneColorClearPass(FrameGraphBuilder& builder, FrameGraphTextureHandle referenceSceneColor);

#pragma once

#include "Renderer/Public/FrameGraph/FrameGraphTextureHandle.h"
#include "Renderer/Public/Viewport/ViewportContracts.h"

class FrameGraphBuilder;

FrameGraphTextureHandle CreateReferenceLightingSample(FrameGraphBuilder& builder, RenderViewportExtent sceneExtent);

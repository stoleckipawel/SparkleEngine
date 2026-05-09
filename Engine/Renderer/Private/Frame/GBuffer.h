#pragma once

#include "FrameGraph/Features/FrameGraphProducts.h"
#include "Renderer/Public/Viewport/ViewportContracts.h"

class FrameGraph;

GBufferTargets BuildGBuffer(FrameGraph& frameGraph, RenderViewportExtent sceneExtent, const SceneTargets& sceneTargets);

#pragma once

#include "FrameGraph/Features/FrameGraphProducts.h"
#include "Renderer/Public/Viewport/ViewportContracts.h"

class FrameGraph;

GBufferTargets CreateGBufferTargets(FrameGraph& frameGraph, RenderViewportExtent sceneExtent, const SceneTargets& sceneTargets);
void AddGBufferPass(FrameGraph& frameGraph, const GBufferTargets& targets);

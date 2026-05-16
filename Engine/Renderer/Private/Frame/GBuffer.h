#pragma once

#include "FrameGraph/Features/FrameGraphProducts.h"
#include "Renderer/Public/Viewport/ViewportContracts.h"

class FrameGraphBuilder;

GBufferTargets CreateGBufferTargets(FrameGraphBuilder& builder, RenderViewportExtent sceneExtent, const SceneTargets& sceneTargets);
void AddGBufferPass(FrameGraphBuilder& builder, const GBufferTargets& targets);

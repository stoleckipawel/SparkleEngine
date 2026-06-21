#pragma once

#include "Frame/Targets/FrameRenderTargets.h"
#include "Renderer/Public/Viewport/ViewportContracts.h"

class FrameGraphBuilder;

GBufferRenderTargets CreateGBufferRenderTargets(FrameGraphBuilder& builder, RenderViewportExtent sceneExtent, const SceneRenderTargets& sceneTargets);
void AddGBufferPass(FrameGraphBuilder& builder, const GBufferRenderTargets& targets);

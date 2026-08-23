#pragma once

#include "Frame/Graph/RenderFrameGraphTargets.h"
#include "Renderer/Public/Viewport/ViewportContracts.h"

class FrameGraphBuilder;

void AddSkyMotionVectorPass(FrameGraphBuilder& builder, RenderViewportExtent sceneExtent, const GBufferRenderTargets& targets);

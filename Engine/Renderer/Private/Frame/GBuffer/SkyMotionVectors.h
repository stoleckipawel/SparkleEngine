#pragma once

#include "Frame/Targets/FrameRenderTargets.h"
#include "Renderer/Public/Viewport/ViewportContracts.h"

class FrameGraphBuilder;

void AddSkyMotionVectorPass(FrameGraphBuilder& builder, RenderViewportExtent sceneExtent, const GBufferRenderTargets& targets);

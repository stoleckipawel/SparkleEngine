#pragma once

#include "FrameGraph/Features/FrameGraphProducts.h"
#include "Renderer/Public/Viewport/ViewportContracts.h"

class FrameGraph;

struct FrameBuildResult
{
	SceneTargets Targets;
	GBufferTargets GBuffer;
};

FrameBuildResult BuildFrame(
    FrameGraph& frameGraph,
    RenderViewportExtent sceneExtent,
    bool presentToBackBuffer);

#pragma once

#include "FrameGraph/Features/FrameGraphProducts.h"
#include "Renderer/Public/Viewport/ViewportContracts.h"

class FrameGraphBuilder;

struct FrameBuildResult
{
	SceneTargets Targets;
	GBufferTargets GBuffer;
};

FrameBuildResult BuildFrame(FrameGraphBuilder& builder, RenderViewportExtent sceneExtent, bool presentToBackBuffer);

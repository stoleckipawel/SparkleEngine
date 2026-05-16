#pragma once

#include "Frame/Targets/FrameRenderTargets.h"
#include "Renderer/Public/Viewport/ViewportContracts.h"

class FrameGraphBuilder;

struct FrameBuildResult
{
	SceneRenderTargets Scene;
	GBufferRenderTargets GBuffer;
};

FrameBuildResult BuildFrame(FrameGraphBuilder& builder, RenderViewportExtent sceneExtent, bool presentToBackBuffer);

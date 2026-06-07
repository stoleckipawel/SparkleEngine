#pragma once

#include "Frame/Targets/FrameRenderTargets.h"
#include "Renderer/Public/FrameGraph/FrameGraphAccelerationStructureHandle.h"
#include "Renderer/Public/Viewport/ViewportContracts.h"

class FrameGraphBuilder;

struct FrameBuildResult
{
	SceneRenderTargets Scene;
	GBufferRenderTargets GBuffer;
	FrameGraphAccelerationStructureHandle SceneTlas = FrameGraphAccelerationStructureHandle::Invalid();
};

FrameBuildResult BuildFrame(FrameGraphBuilder& builder, RenderViewportExtent sceneExtent, bool presentToBackBuffer);

#pragma once

#include "Resources/History/FrameHistory.h"
#include "Renderer/Public/FrameGraph/FrameGraphTextureHandle.h"
#include "Renderer/Public/Viewport/ViewportContracts.h"

class FrameGraphBuilder;

void AddExposurePass(
	FrameGraphBuilder& builder,
	RenderViewportExtent sceneExtent,
	FrameGraphTextureHandle finalSceneColor,
	const FrameGraphTextureHistory& history,
	FrameGraphTextureHandle exposure);

#pragma once

#include "FrameGraphProducts.h"
#include "Renderer/Public/Viewport/ViewportContracts.h"

class FrameGraph;
class Window;

namespace FrameGraphFeatures
{
	FrameGraphSceneTargets CreateSceneTargets(FrameGraph& frameGraph, const Window& window, const RenderViewportExtent& sceneExtent);

	void AddCopyToBackBufferPass(FrameGraph& frameGraph, const FrameGraphSceneTargets& sceneTargets);
}  // namespace FrameGraphFeatures

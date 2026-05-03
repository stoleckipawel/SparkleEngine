#pragma once

#include "FrameGraphProducts.h"
#include "Renderer/Public/Viewport/ViewportContracts.h"

class FrameGraph;
class Window;

namespace FrameGraphFeatures
{
	FrameGraphGBufferTargets AddGBufferPass(
	    FrameGraph& frameGraph,
	    const Window& window,
	    const RenderViewportExtent& sceneExtent,
	    const FrameGraphSceneTargets& sceneTargets);
}

#pragma once

#include "FrameGraphProducts.h"

#include "Renderer/Public/Overlays/RendererOverlay.h"

class FrameGraph;
class Window;

namespace FrameGraphFeatures
{
	FrameGraphSceneTargets CreateSceneTargets(FrameGraph& frameGraph, const Window& window);

	void AddCopyToBackBufferPass(
	    FrameGraph& frameGraph,
	    const FrameGraphPresentationInputs& presentation,
	    const FrameGraphComputeShowcaseOutputs& computeOutputs);

	void AddUiCompositionPass(
	    FrameGraph& frameGraph,
	    IRendererOverlay& overlay,
	    const FrameGraphPresentationInputs& presentation);
}  // namespace FrameGraphFeatures

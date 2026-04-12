#pragma once

#include "FrameGraphProducts.h"

class FrameGraph;
class Window;

namespace FrameGraphFeatures
{
	FrameGraphSceneTargets CreateSceneTargets(FrameGraph& frameGraph, const Window& window);

	void AddCopyToBackBufferPass(
	    FrameGraph& frameGraph,
	    const FrameGraphPresentationInputs& presentation,
	    const FrameGraphComputeShowcaseOutputs& computeOutputs);
}  // namespace FrameGraphFeatures

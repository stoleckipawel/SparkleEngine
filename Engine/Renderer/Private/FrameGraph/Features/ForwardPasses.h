#pragma once

#include "FrameGraphProducts.h"

class FrameGraph;

namespace FrameGraphFeatures
{
	void AddForwardOpaquePass(
	    FrameGraph& frameGraph,
	    const FrameGraphSceneTargets& sceneTargets,
	    const FrameGraphShadowOutputs& shadowOutputs);
}

#pragma once

#include "FrameGraphProducts.h"

class FrameGraph;
class UI;
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
    UI& ui,
    const FrameGraphPresentationInputs& presentation);
}

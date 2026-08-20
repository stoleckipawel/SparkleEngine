#pragma once

#include "Frame/Core/FrameAssembly.h"

class FrameGraphBuilder;

void AddReferenceLightingProducerPasses(
    FrameGraphBuilder& builder,
    RenderViewportExtent sceneExtent,
    const FrameAssemblyResourceLayout& resources);
void FinalizeReferenceLightingPasses(
    FrameGraphBuilder& builder,
    RenderViewportExtent sceneExtent,
    FrameGraphTextureHandle referenceSample,
    const FrameAssemblyResourceLayout& resources);

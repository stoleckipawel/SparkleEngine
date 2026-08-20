#pragma once

#include "Frame/Core/FrameAssembly.h"

class FrameGraphBuilder;

void AddReferenceLightingAccumulationPass(
    FrameGraphBuilder& builder,
    RenderViewportExtent sceneExtent,
    FrameGraphTextureHandle referenceSample,
    const FrameAssemblyResourceLayout& resources);

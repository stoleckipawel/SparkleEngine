#pragma once

#include "Frame/Core/FrameAssembly.h"

class FrameGraphBuilder;

void AddRestirIndirectResolvePass(
    FrameGraphBuilder& builder,
    RenderViewportExtent sceneExtent,
    const FrameAssemblyResourceLayout& resources);

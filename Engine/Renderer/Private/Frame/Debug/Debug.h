#pragma once

#include "Frame/Core/FrameAssembly.h"
#include "Frame/Targets/FrameRenderTargets.h"

class FrameGraphBuilder;

void AddDebugPasses(
    FrameGraphBuilder& builder,
    const FrameAssemblyResourceLayout& resources);

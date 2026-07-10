#pragma once

#include "Frame/Core/FrameAssembly.h"

class FrameGraphBuilder;

void AddReferenceLightingAccumulationPass(
    FrameGraphBuilder& builder,
    FrameGraphTextureHandle referenceSample,
    const FrameAssemblyResourceLayout& resources);

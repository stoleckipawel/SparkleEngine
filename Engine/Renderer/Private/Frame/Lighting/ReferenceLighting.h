#pragma once

#include "Frame/Core/FrameAssembly.h"

class FrameGraphBuilder;

void AddReferenceLightingProducerPasses(FrameGraphBuilder& builder, const FrameAssemblyResourceLayout& resources);
void FinalizeReferenceLightingPasses(
    FrameGraphBuilder& builder,
    FrameGraphTextureHandle referenceSample,
    const FrameAssemblyResourceLayout& resources);

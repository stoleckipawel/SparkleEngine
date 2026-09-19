#pragma once

#include "Passes/PostProcessing/ExposureMomentResources.h"

class FrameGraphBuilder;
struct RenderFrameGraphResources;

void AddExposureAdaptationPass(
    FrameGraphBuilder& builder,
    const ExposureMomentTexture& luminanceMoments,
    const RenderFrameGraphResources& resources);

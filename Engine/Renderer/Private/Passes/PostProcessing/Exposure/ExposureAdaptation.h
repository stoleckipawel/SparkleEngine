#pragma once

struct RenderFrame;

#include "Passes/PostProcessing/Exposure/ExposureMomentResources.h"

class FrameGraphBuilder;
struct RenderFrameGraphResources;

void AddExposureAdaptationPass(
    FrameGraphBuilder& builder,
    const RenderFrame& frame,
    const ExposureMomentTexture& luminanceMoments,
    const RenderFrameGraphResources& resources);

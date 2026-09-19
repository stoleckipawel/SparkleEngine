#pragma once

#include "Passes/PostProcessing/ExposureMomentResources.h"
#include "Renderer/Public/Viewport/ViewportContracts.h"

class FrameGraphBuilder;
struct RenderFrameGraphResources;

ExposureMomentTexture BuildExposureReductionMoments(
    FrameGraphBuilder& builder,
    RenderViewportExtent sceneExtent,
    const RenderFrameGraphResources& resources);
ExposureMomentTexture BuildExposureDownsampleMoments(
    FrameGraphBuilder& builder,
    RenderViewportExtent sceneExtent,
    const RenderFrameGraphResources& resources);

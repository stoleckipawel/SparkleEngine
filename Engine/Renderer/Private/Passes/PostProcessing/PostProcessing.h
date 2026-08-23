#pragma once

#include "Frame/Graph/BuildRenderFrameGraph.h"

class FrameGraphBuilder;
class IUpscalerProvider;

void AddPreReconstructionPostProcessingPasses(
    FrameGraphBuilder& builder,
    const RenderFrameGraphSettings& settings,
    RenderFrameGraphResources& resources);

void AddPostProcessingPasses(
    FrameGraphBuilder& builder,
    const RenderFrameGraphSettings& settings,
    IUpscalerProvider* upscalerProvider,
    RenderFrameGraphResources& resources);

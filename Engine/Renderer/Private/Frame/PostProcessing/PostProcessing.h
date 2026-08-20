#pragma once

#include "Frame/Core/Frame.h"

class FrameGraphBuilder;
class IUpscalerProvider;

void AddPreReconstructionPostProcessingPasses(
    FrameGraphBuilder& builder,
    const FrameBuildSettings& settings,
    FrameAssemblyResourceLayout& resources);

void AddPostProcessingPasses(
    FrameGraphBuilder& builder,
    const FrameBuildSettings& settings,
    IUpscalerProvider* upscalerProvider,
    FrameAssemblyResourceLayout& resources);

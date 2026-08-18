#pragma once

#include "Frame/Core/Frame.h"

class FrameGraphBuilder;

void AddPreReconstructionPostProcessingPasses(
    FrameGraphBuilder& builder,
    const FrameBuildSettings& settings,
    FrameAssemblyResourceLayout& resources);

void AddPostProcessingPasses(FrameGraphBuilder& builder, const FrameBuildSettings& settings, FrameAssemblyResourceLayout& resources);

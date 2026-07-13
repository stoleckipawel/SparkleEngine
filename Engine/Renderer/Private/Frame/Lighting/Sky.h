#pragma once

#include "Frame/Targets/FrameRenderTargets.h"

class FrameGraphBuilder;

void AddSkyPass(
    FrameGraphBuilder& builder,
    FrameGraphTextureHandle output,
    FrameGraphTextureHandle sceneDepth,
    FrameGraphTextureHandle sky);

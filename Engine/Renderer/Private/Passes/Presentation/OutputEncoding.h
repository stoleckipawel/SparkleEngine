#pragma once

#include "Frame/Graph/RenderFrameGraphSettings.h"
#include "FrameGraph/FrameGraphTextureHandle.h"

class FrameGraphBuilder;

FrameGraphTextureHandle AddOutputEncodingPass(
    FrameGraphBuilder& builder,
    const RenderFrameGraphSettings& settings,
    FrameGraphTextureHandle displayLinearColor);

#pragma once

#include "FrameGraph/FrameGraphTextureHandle.h"

class FrameGraphBuilder;
struct RenderFrameGraphResources;
struct RenderFrameGraphSettings;

void AddPresentationOutputPass(
    FrameGraphBuilder& builder,
    const RenderFrameGraphSettings& settings,
    FrameGraphTextureHandle encodedColor,
    RenderFrameGraphResources& resources);

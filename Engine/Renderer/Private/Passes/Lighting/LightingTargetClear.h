#pragma once
#include "FrameGraph/FrameGraphTextureHandle.h"
#include <span>
#include <string_view>

class FrameGraphBuilder;
struct RenderViewportExtent;

void AddLightingTargetClearPass(
    FrameGraphBuilder& builder,
    std::string_view name,
    RenderViewportExtent extent,
    std::span<const FrameGraphTextureHandle> targets);

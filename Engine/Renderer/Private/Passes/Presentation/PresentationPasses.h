#pragma once

#include "Frame/Graph/RenderFrameGraphSettings.h"
#include "Renderer/Public/Viewport/RenderViewMode.h"

class FrameGraphBuilder;
struct RenderFrameGraphResources;

void AddPresentationPasses(
    FrameGraphBuilder& builder,
    const RenderFrameGraphSettings& settings,
    RenderViewMode viewMode,
    RenderFrameGraphResources& resources);

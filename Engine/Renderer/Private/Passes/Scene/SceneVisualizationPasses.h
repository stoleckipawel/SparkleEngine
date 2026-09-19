#pragma once

#include "Renderer/Public/Viewport/RenderViewMode.h"

class FrameGraphBuilder;
struct RenderFrameGraphResources;
struct RenderFrameGraphSettings;

void AddSceneVisualizationPasses(
    FrameGraphBuilder& builder,
    const RenderFrameGraphSettings& settings,
    RenderViewMode viewMode,
    RenderFrameGraphResources& resources);

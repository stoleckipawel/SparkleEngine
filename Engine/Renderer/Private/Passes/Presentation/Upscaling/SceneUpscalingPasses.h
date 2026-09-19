#pragma once

#include "Renderer/Public/Viewport/RenderViewMode.h"

class FrameGraphBuilder;
class RendererImageProviderStack;
struct RenderFrameGraphResources;
struct RenderFrameGraphSettings;

void AddSceneUpscalingPasses(
    FrameGraphBuilder& builder,
    const RenderFrameGraphSettings& settings,
    RenderViewMode viewMode,
    RendererImageProviderStack& imageProviders,
    RenderFrameGraphResources& resources);

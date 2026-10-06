#pragma once

struct RenderFrame;

#include "Renderer/Public/Viewport/RenderViewMode.h"
#include "Renderer/Public/Viewport/ViewportContracts.h"

class FrameGraphBuilder;
struct RenderFrameGraphResources;
struct ViewportFrameProducts;

bool PrepareLightingVisualizationProducts(
    RenderViewMode viewMode,
    const RenderFrameGraphResources& resources,
    ViewportFrameProducts& products) noexcept;

void AddLightingVisualizationPass(
    FrameGraphBuilder& builder,
    const RenderFrame& frame,
    RenderViewportExtent sceneExtent,
    const RenderFrameGraphResources& resources);

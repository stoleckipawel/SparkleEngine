#pragma once

#include "Renderer/Public/Viewport/RenderViewMode.h"
#include "Renderer/Public/Viewport/ViewportContracts.h"

class FrameGraphBuilder;
struct RenderFrameGraphResources;
struct ViewportFrameProducts;

bool PrepareLightingVisualizationProducts(
    RenderViewMode viewMode,
    const RenderFrameGraphResources& resources,
    ViewportFrameProducts& products) noexcept;

void AddLightingVisualizationPass(FrameGraphBuilder& builder, RenderViewportExtent sceneExtent, const RenderFrameGraphResources& resources);

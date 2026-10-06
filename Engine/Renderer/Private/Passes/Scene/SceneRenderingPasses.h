#pragma once

#include "Renderer/Public/Viewport/RenderViewMode.h"
#include <cstdint>

struct RenderView;
struct RenderFrameGraphResources;
struct ViewportFrameProducts;

std::uint64_t GetSceneRenderingGraphRebuildKey(RenderViewMode viewMode) noexcept;
bool PrepareSceneRenderingProducts(
    const RenderView& view,
    const RenderFrameGraphResources& resources,
    ViewportFrameProducts& products) noexcept;

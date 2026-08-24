#pragma once

#include "Frame/Graph/RenderFrameGraphResources.h"
#include "Renderer/Public/Viewport/ViewportContracts.h"

void PublishViewportRenderProducts(
    ViewportRenderProducts& products,
    const ViewportRenderRequest& request,
    const ViewportFrameProducts& frameProducts,
    RenderViewportExtent renderExtent,
    RenderViewportExtent outputExtent) noexcept;

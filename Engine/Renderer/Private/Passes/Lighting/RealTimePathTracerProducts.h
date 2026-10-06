#pragma once
#include "Renderer/Public/Viewport/ViewportContracts.h"

struct RenderFrameGraphResources;
struct ViewportFrameProducts;

bool PrepareRealTimePathTracerProducts(RenderViewMode viewMode, ViewportFrameProducts& products) noexcept;

void PublishRealTimePathTracerProducts(RenderFrameGraphResources& resources);

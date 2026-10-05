#pragma once

struct RenderFrameGraphResources;
struct RenderFrameGraphSettings;
struct ViewportFrameProducts;

bool HasRequiredRealTimePathTracerProducts(const RenderFrameGraphSettings& settings) noexcept;
bool PrepareRealTimePathTracerProducts(const RenderFrameGraphSettings& settings, ViewportFrameProducts& products) noexcept;

void PublishRealTimePathTracerProducts(RenderFrameGraphResources& resources);

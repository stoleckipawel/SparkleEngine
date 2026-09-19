#pragma once

class FrameGraphBuilder;
struct RenderFrameGraphResources;
struct RenderFrameGraphSettings;

void ExportViewportFrameProducts(
    FrameGraphBuilder& builder,
    const RenderFrameGraphSettings& settings,
    const RenderFrameGraphResources& resources) noexcept;

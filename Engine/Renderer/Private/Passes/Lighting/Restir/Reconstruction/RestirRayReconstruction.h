#pragma once

class FrameGraphBuilder;
class RendererImageProviderStack;
struct RenderFrameGraphResources;
struct RenderFrameGraphSettings;

void AddRestirRayReconstructionPass(
    FrameGraphBuilder& builder,
    const RenderFrameGraphSettings& settings,
    RendererImageProviderStack& imageProviders,
    RenderFrameGraphResources& resources);

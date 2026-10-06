#pragma once
#include "Renderer/Public/Viewport/ViewportContracts.h"

class FrameGraphBuilder;
class RendererImageProviderStack;
struct RenderFrame;
struct RenderFrameGraphResources;

void AddRestirRayReconstructionPass(
    FrameGraphBuilder& builder,
    const RenderFrame& frame,
    RenderViewportExtent sceneExtent,
    RendererImageProviderStack& imageProviders,
    RenderFrameGraphResources& resources);

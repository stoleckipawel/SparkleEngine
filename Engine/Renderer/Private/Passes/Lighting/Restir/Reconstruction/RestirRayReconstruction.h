#pragma once
#include "Renderer/Public/Viewport/ViewportContracts.h"

class FrameGraphBuilder;
class RendererImageProviderStack;
struct RenderFrameGraphResources;

void AddRestirRayReconstructionPass(
    FrameGraphBuilder& builder,
    RenderViewportExtent sceneExtent,
    RendererImageProviderStack& imageProviders,
    RenderFrameGraphResources& resources);

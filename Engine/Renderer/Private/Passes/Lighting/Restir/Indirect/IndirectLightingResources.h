#pragma once

#include "Renderer/Public/Viewport/ViewportContracts.h"

class FrameGraphBuilder;
struct RenderFrameGraphResources;

void CreateIndirectLightingResources(
    FrameGraphBuilder& builder,
    RenderViewportExtent sceneExtent,
    bool createRayReconstructionGuides,
    RenderFrameGraphResources& resources);

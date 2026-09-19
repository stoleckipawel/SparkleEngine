#pragma once

#include "Frame/Graph/RenderFrameGraphResources.h"
#include "Renderer/Public/Viewport/ViewportContracts.h"

class FrameGraphBuilder;
class RenderRayTracingScene;

void AddRestirDirectLightingPasses(
    FrameGraphBuilder& builder,
    RenderViewportExtent sceneExtent,
    RenderRayTracingScene& rayTracingScene,
    RenderFrameGraphResources& resources);

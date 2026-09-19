#pragma once

#include "Renderer/Public/Viewport/ViewportContracts.h"

class FrameGraphBuilder;
class RenderRayTracingScene;
struct RenderFrameGraphResources;

void AddRealTimeLightingPasses(
    FrameGraphBuilder& builder,
    RenderRayTracingScene& rayTracingScene,
    RenderViewportExtent sceneExtent,
    RenderFrameGraphResources& resources);

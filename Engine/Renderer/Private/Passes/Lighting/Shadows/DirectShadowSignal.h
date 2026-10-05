#pragma once

#include "Renderer/Public/Viewport/ViewportContracts.h"

class FrameGraphBuilder;
class RenderRayTracingScene;
struct RenderFrameGraphResources;

void AddDirectShadowSignalPass(
    FrameGraphBuilder& builder,
    RenderViewportExtent sceneExtent,
    RenderFrameGraphResources& resources,
    RenderRayTracingScene& rayTracingScene);

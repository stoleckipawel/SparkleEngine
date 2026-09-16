#pragma once

#include "Renderer/Public/Viewport/ViewportContracts.h"

class FrameGraphBuilder;
class RenderRayTracingScene;
struct DirectShadowSignalResources;
struct RenderFrameGraphResources;

void AddDirectShadowSignalPass(
    FrameGraphBuilder& builder,
    RenderViewportExtent sceneExtent,
    const RenderFrameGraphResources& resources,
    const DirectShadowSignalResources& shadowSignals,
    RenderRayTracingScene& rayTracingScene);

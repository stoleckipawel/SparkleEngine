#pragma once

#include "Renderer/Public/Settings/EngineRenderingRayTracingTypes.h"
#include "Renderer/Public/Viewport/ViewportContracts.h"

class FrameGraphBuilder;
struct RenderFrameGraphResources;

void CreateGBufferRenderTargets(
    FrameGraphBuilder& builder,
    RenderViewportExtent sceneExtent,
    GBufferAlgorithm algorithm,
    RenderFrameGraphResources& resources);

#pragma once

#include "Renderer/Public/Viewport/ViewportContracts.h"

class FrameGraphBuilder;
struct DirectShadowSignalResources;
struct RenderFrameGraphResources;

void AddDirectLightReservoirTemporalPass(
    FrameGraphBuilder& builder,
    RenderViewportExtent sceneExtent,
    const RenderFrameGraphResources& resources,
    const DirectShadowSignalResources& shadowSignals);
void AddDirectLightReservoirSpatialPass(
    FrameGraphBuilder& builder,
    RenderViewportExtent sceneExtent,
    const RenderFrameGraphResources& resources,
    const DirectShadowSignalResources& shadowSignals);

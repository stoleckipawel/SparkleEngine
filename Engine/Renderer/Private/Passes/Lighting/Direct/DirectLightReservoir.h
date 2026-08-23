#pragma once

#include "Frame/Graph/RenderFrameGraphTargets.h"
#include "Renderer/Public/Viewport/ViewportContracts.h"

class FrameGraphBuilder;
struct DirectShadowSignalResources;
struct RenderFrameGraphImportedSceneResources;

void AddDirectLightReservoirPasses(
    FrameGraphBuilder& builder,
    RenderViewportExtent sceneExtent,
    const SceneRenderTargets& sceneTargets,
    const GBufferRenderTargets& gbuffer,
    const DirectShadowSignalResources& shadowSignals,
    const RenderFrameGraphImportedSceneResources& externalResources);

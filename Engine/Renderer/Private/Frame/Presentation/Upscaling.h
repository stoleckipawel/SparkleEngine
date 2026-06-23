#pragma once

#include "Frame/Core/FrameAssembly.h"
#include "Frame/Targets/FrameRenderTargets.h"
#include "Renderer/Public/Viewport/ViewportContracts.h"

class FrameGraphBuilder;

FrameAssemblyProviderResources CreateUpscalerProviderInputs(
    const SceneRenderTargets& sceneTargets,
    const GBufferRenderTargets& gbuffer);

void AddUpscalerEvaluationPass(
    FrameGraphBuilder& builder,
    RenderViewportExtent sceneExtent,
    const SceneRenderTargets& sceneTargets,
    const GBufferRenderTargets& gbuffer);

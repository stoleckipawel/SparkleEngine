#pragma once

#include "Frame/Core/FrameAssembly.h"
#include "Frame/Targets/FrameRenderTargets.h"
#include "Renderer/Public/Viewport/ViewportContracts.h"
#include "Upscaling/UpscalerInputContract.h"

#include <cstdint>

class FrameGraphBuilder;

UpscalerInputContract BuildFrameUpscalerInputContract(
    const FrameAssemblyUpscalerProviderResources& providerInputs,
    RenderViewportExtent sceneExtent,
    std::uint64_t frameIndex,
    const PerViewCameraConstantBufferData& camera,
    const PerTemporalConstantBufferData& temporalData,
    RenderTemporalFrameState temporalState);

void AddUpscalerEvaluationPass(
    FrameGraphBuilder& builder,
    RenderViewportExtent sceneExtent,
    const SceneRenderTargets& sceneTargets,
    const GBufferRenderTargets& gbuffer);

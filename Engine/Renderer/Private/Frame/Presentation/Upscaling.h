#pragma once

#include "Frame/Core/FrameAssembly.h"
#include "Frame/Core/FrameProviderResources.h"
#include "Renderer/Public/Viewport/ViewportContracts.h"
#include "Upscaling/UpscalerInputContract.h"

#include <cstdint>

class FrameGraphBuilder;

UpscalerInputContract BuildFrameUpscalerInputContract(
    const FrameUpscalerProviderResources& providerInputs,
    RenderViewportExtent renderExtent,
    RenderViewportExtent outputExtent,
    std::uint64_t frameIndex,
    const PerViewCameraConstantBufferData& camera,
    const PerTemporalConstantBufferData& temporalData,
    RenderTemporalFrameState temporalState);

void AddUpscalingPasses(
    FrameGraphBuilder& builder,
    RenderViewportExtent renderExtent,
    RenderViewportExtent outputExtent,
    FrameAssemblyResourceLayout& resources);

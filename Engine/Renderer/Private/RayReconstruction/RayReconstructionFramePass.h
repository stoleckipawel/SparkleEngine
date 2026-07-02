#pragma once

#include "Frame/Core/FrameProviderResources.h"
#include "RayReconstruction/RayReconstructionInputContract.h"
#include "Renderer/Public/Viewport/ViewportContracts.h"

#include <cstdint>

class FrameGraphBuilder;
struct PerTemporalConstantBufferData;
struct PerViewCameraConstantBufferData;
struct RenderTemporalFrameState;

bool HasRequiredRayReconstructionProviderResources(const FrameRayReconstructionProviderResources& providerInputs) noexcept;

RayReconstructionInputContract BuildFrameRayReconstructionInputContract(
    const FrameRayReconstructionProviderResources& providerInputs,
    RenderViewportExtent sceneExtent,
    std::uint64_t frameIndex,
    const PerViewCameraConstantBufferData& camera,
    const PerTemporalConstantBufferData& temporalData,
    RenderTemporalFrameState temporalState);

void AddRayReconstructionProviderPass(
    FrameGraphBuilder& builder,
    const char* passName,
    RenderViewportExtent sceneExtent,
    const FrameRayReconstructionProviderResources& providerInputs);

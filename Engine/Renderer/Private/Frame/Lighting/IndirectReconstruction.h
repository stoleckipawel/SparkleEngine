#pragma once

#include "Frame/Core/FrameAssembly.h"
#include "Frame/Core/FrameProviderResources.h"
#include "Frame/Targets/FrameRenderTargets.h"
#include "RayReconstruction/RayReconstructionInputContract.h"
#include "Renderer/Public/FrameGraph/FrameGraphTextureHandle.h"
#include "Renderer/Public/Viewport/ViewportContracts.h"

#include <cstdint>

class FrameGraphBuilder;

FrameRayReconstructionProviderResources BuildIndirectRayReconstructionProviderInputs(
    const SceneRenderTargets& sceneTargets,
    const GBufferRenderTargets& gbuffer,
    const LightingRenderTargets& lighting,
    FrameGraphTextureHandle exposure);

RayReconstructionInputContract BuildFrameIndirectRayReconstructionInputContract(
    const FrameRayReconstructionProviderResources& providerInputs,
    RenderViewportExtent sceneExtent,
    std::uint64_t frameIndex,
    const PerViewCameraConstantBufferData& camera,
    const PerTemporalConstantBufferData& temporalData,
    RenderTemporalFrameState temporalState);

void AddIndirectRayReconstructionPass(
    FrameGraphBuilder& builder,
    RenderViewportExtent sceneExtent,
    const FrameRayReconstructionProviderResources& providerInputs);

bool AddIndirectRayReconstructionPassIfEnabled(
    FrameGraphBuilder& builder,
    RenderViewportExtent sceneExtent,
    FrameAssemblyResourceLayout& resources);

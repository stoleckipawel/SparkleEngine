#pragma once

#include "Frame/Core/FrameAssembly.h"
#include "Frame/Targets/FrameRenderTargets.h"
#include "Renderer/Public/FrameGraph/FrameGraphTextureHandle.h"

FrameAssemblyUpscalerProviderResources BuildFrameUpscalerProviderInputs(
    const SceneRenderTargets& sceneTargets,
    const GBufferRenderTargets& gbuffer,
    FrameGraphTextureHandle exposure);

FrameAssemblyDenoiserProviderResources BuildFrameDenoiserProviderInputs(
    const SceneRenderTargets& sceneTargets,
    const GBufferRenderTargets& gbuffer,
    const LightingRenderTargets& lighting,
    FrameGraphTextureHandle exposure);

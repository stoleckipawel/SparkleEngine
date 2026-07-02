#pragma once

#include "Frame/Core/FrameProviderResources.h"
#include "Frame/Targets/FrameRenderTargets.h"
#include "Renderer/Public/FrameGraph/FrameGraphTextureHandle.h"

FrameUpscalerProviderResources BuildFrameUpscalerProviderInputs(
    const SceneRenderTargets& sceneTargets,
    const GBufferRenderTargets& gbuffer,
    FrameGraphTextureHandle exposure);

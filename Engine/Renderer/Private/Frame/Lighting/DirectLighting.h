#pragma once

#include "Frame/Targets/FrameRenderTargets.h"

class FrameGraphBuilder;
struct DirectShadowSignalResources;
struct FrameAssemblyExternalResources;

void AddDirectLightingPass(
    FrameGraphBuilder& builder,
    const LightingRenderTargets& lighting,
    const SceneRenderTargets& sceneTargets,
    const GBufferRenderTargets& gbuffer,
    const DirectShadowSignalResources& shadowSignals,
    const FrameAssemblyExternalResources& externalResources);

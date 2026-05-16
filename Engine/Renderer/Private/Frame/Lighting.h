#pragma once

#include "Frame/Targets/FrameRenderTargets.h"

class FrameGraphBuilder;

void AddLightingPasses(FrameGraphBuilder& builder, const SceneRenderTargets& sceneTargets, const LightingRenderTargets& lighting, const GBufferRenderTargets& gbuffer);

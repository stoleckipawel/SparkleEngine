#pragma once

#include "Frame/Targets/FrameRenderTargets.h"

class FrameGraphBuilder;

void AddSkyPass(FrameGraphBuilder& builder, const SceneRenderTargets& sceneTargets, const GBufferRenderTargets& gbuffer);

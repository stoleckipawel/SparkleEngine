#pragma once

#include "Frame/Targets/FrameRenderTargets.h"

class FrameGraphBuilder;

void AddDirectLightingPass(FrameGraphBuilder& builder, const LightingRenderTargets& lighting, const GBufferRenderTargets& gbuffer);

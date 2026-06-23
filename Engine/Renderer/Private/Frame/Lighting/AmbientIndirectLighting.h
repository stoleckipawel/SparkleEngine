#pragma once

#include "Frame/Targets/FrameRenderTargets.h"

class FrameGraphBuilder;

void AddAmbientIndirectLightingPass(FrameGraphBuilder& builder, const LightingRenderTargets& lighting, const GBufferRenderTargets& gbuffer);

#pragma once

#include "FrameGraph/Features/FrameGraphProducts.h"

class FrameGraphBuilder;

void AddDirectLightingPass(FrameGraphBuilder& builder, const LightingTargets& lighting, const GBufferTargets& gbuffer);

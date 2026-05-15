#pragma once

#include "FrameGraph/Features/FrameGraphProducts.h"

class FrameGraph;

void AddDirectLightingPass(FrameGraph& frameGraph, const LightingTargets& lighting, const GBufferTargets& gbuffer);

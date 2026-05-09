#pragma once

#include "FrameGraph/Features/FrameGraphProducts.h"

class FrameGraph;

void BuildDirectLighting(FrameGraph& frameGraph, const LightingTargets& lighting, const GBufferTargets& gbuffer);

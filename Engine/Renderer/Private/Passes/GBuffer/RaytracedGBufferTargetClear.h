#pragma once

#include "Frame/Graph/RenderFrameGraphTargets.h"

class FrameGraphBuilder;

void AddRaytracedGBufferTargetClearPass(FrameGraphBuilder& builder, const GBufferRenderTargets& targets);

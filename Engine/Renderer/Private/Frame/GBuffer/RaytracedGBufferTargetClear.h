#pragma once

#include "Frame/Targets/FrameRenderTargets.h"

class FrameGraphBuilder;

void AddRaytracedGBufferTargetClearPass(FrameGraphBuilder& builder, const GBufferRenderTargets& targets);

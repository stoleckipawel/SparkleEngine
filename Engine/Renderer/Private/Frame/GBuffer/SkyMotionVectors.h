#pragma once

#include "Frame/Targets/FrameRenderTargets.h"

class FrameGraphBuilder;

void AddSkyMotionVectorPass(FrameGraphBuilder& builder, const GBufferRenderTargets& targets);

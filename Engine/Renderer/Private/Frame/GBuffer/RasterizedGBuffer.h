#pragma once

#include "Frame/Targets/FrameRenderTargets.h"

class FrameGraphBuilder;

void AddRasterizedGBufferPass(FrameGraphBuilder& builder, const GBufferRenderTargets& targets);

#pragma once

#include "Frame/Targets/FrameRenderTargets.h"

class FrameGraphBuilder;

void AddIndirectLightingPass(FrameGraphBuilder& builder, const LightingRenderTargets& lighting);

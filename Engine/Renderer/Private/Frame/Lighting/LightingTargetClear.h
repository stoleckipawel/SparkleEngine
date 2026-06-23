#pragma once

#include "Frame/Targets/FrameRenderTargets.h"

class FrameGraphBuilder;

void AddLightingTargetClearPass(FrameGraphBuilder& builder, const LightingRenderTargets& lighting);

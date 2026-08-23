#pragma once

#include "Frame/Graph/RenderFrameGraphTargets.h"

class FrameGraphBuilder;

void AddLightingTargetClearPass(FrameGraphBuilder& builder, const LightingRenderTargets& lighting);

#pragma once

#include "FrameGraph/Features/FrameGraphProducts.h"

class FrameGraphBuilder;

void AddIndirectLightingPass(FrameGraphBuilder& builder, const LightingTargets& lighting);

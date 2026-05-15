#pragma once

#include "FrameGraph/Features/FrameGraphProducts.h"

class FrameGraph;

void AddIndirectLightingPass(FrameGraph& frameGraph, const LightingTargets& lighting);
#pragma once

#include "FrameGraph/Features/FrameGraphProducts.h"

class FrameGraph;

void AddSkyPass(FrameGraph& frameGraph, const SceneTargets& sceneTargets, const GBufferTargets& gbuffer);
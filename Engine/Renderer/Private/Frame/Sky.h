#pragma once

#include "FrameGraph/Features/FrameGraphProducts.h"

class FrameGraph;

void BuildSky(FrameGraph& frameGraph, const SceneTargets& sceneTargets, const GBufferTargets& gbuffer);
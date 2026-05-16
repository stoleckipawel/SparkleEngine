#pragma once

#include "FrameGraph/Features/FrameGraphProducts.h"

class FrameGraphBuilder;

void AddSkyPass(FrameGraphBuilder& builder, const SceneTargets& sceneTargets, const GBufferTargets& gbuffer);

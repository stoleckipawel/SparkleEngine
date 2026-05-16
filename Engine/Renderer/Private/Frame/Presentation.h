#pragma once

#include "Frame/Targets/FrameRenderTargets.h"

class FrameGraphBuilder;

void AddPresentationPass(FrameGraphBuilder& builder, const SceneRenderTargets& sceneTargets);

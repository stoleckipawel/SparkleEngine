#pragma once

#include "Frame/Graph/RenderFrameGraphResources.h"
#include "Frame/Graph/RenderFrameGraphTargets.h"

class FrameGraphBuilder;

void AddDebugPasses(FrameGraphBuilder& builder, RenderViewportExtent sceneExtent, const RenderFrameGraphResources& resources);

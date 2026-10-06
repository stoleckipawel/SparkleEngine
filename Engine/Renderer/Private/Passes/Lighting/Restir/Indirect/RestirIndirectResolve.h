#pragma once

#include "Frame/Graph/RenderFrameGraphResources.h"

class FrameGraphBuilder;

void AddRestirIndirectResolvePass(FrameGraphBuilder& builder, RenderViewportExtent sceneExtent, const RenderFrameGraphResources& resources);

#pragma once

#include "Frame/Graph/BuildRenderFrameGraph.h"

class FrameGraphBuilder;

void AddPresentationPasses(FrameGraphBuilder& builder, const RenderFrameGraphSettings& settings, RenderFrameGraphResources& resources);

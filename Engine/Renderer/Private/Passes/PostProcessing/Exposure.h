#pragma once

#include "Frame/Graph/BuildRenderFrameGraph.h"

class FrameGraphBuilder;

void AddExposurePass(FrameGraphBuilder& builder, const RenderFrameGraphSettings& settings, const RenderFrameGraphResources& resources);

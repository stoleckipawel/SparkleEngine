#pragma once

#include "Frame/Graph/RenderFrameGraphSettings.h"

class FrameGraphBuilder;

void AddPostProcessingPasses(FrameGraphBuilder& builder, const RenderFrameGraphSettings& settings, RenderFrameGraphResources& resources);

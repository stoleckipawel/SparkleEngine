#pragma once

#include "Frame/Graph/RenderFrameGraphResources.h"

class FrameGraphBuilder;
struct RenderFrameGraphSettings;

void AddReferencePathTracerPasses(
    FrameGraphBuilder& builder,
    const RenderFrameGraphSettings& settings,
    RenderFrameGraphResources& resources);

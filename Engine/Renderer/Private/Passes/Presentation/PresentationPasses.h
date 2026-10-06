#pragma once

struct RenderFrame;

#include "Frame/Graph/RenderFrameGraphSettings.h"

class FrameGraphBuilder;
struct RenderFrameGraphResources;

void AddPresentationPasses(
    FrameGraphBuilder& builder,
    const RenderFrame& frame,
    const RenderFrameGraphSettings& settings,
    RenderFrameGraphResources& resources);

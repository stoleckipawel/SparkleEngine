#pragma once

#include "Frame/Graph/RenderFrameGraphSettings.h"

class FrameGraphBuilder;
struct RenderFrameGraphResources;
struct ViewportRenderRequest;

void AddPresentationPasses(
    FrameGraphBuilder& builder,
    const RenderFrameGraphSettings& settings,
    const ViewportRenderRequest& viewport,
    RenderFrameGraphResources& resources);

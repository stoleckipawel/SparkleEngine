#pragma once

#include "Renderer/Public/Viewport/ViewportContracts.h"

class FrameGraphBuilder;
struct ReferencePathTracerGraphResources;
struct ReferencePathTracerUniformData;
struct RenderFrameGraphResources;

void AddReferencePathTracerDisplayPass(
    FrameGraphBuilder& builder,
    RenderViewportExtent extent,
    const RenderFrameGraphResources& resources,
    const ReferencePathTracerGraphResources& graphResources,
    const ReferencePathTracerUniformData& uniformData);

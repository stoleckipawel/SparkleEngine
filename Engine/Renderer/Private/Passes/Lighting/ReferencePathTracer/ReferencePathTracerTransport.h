#pragma once

#include "Renderer/Public/Viewport/ViewportContracts.h"

class FrameGraphBuilder;
class RenderRayTracingScene;
struct ReferencePathTracerGraphResources;
struct ReferencePathTracerUniformData;
struct RenderFrameGraphResources;

void AddReferencePathTracerTransportPass(
    FrameGraphBuilder& builder,
    RenderViewportExtent extent,
    const RenderFrameGraphResources& resources,
    const ReferencePathTracerGraphResources& graphResources,
    const ReferencePathTracerUniformData& uniformData,
    RenderRayTracingScene& rayTracingScene);

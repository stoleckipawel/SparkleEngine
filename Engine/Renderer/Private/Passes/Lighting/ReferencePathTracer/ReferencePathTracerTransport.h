#pragma once

struct RenderFrame;

#include "Renderer/Public/Viewport/ViewportContracts.h"

class FrameGraphBuilder;
class RenderRayTracingScene;
struct ReferencePathTracerGraphResources;
struct ReferencePathTracerWork;
struct RenderFrameGraphResources;

void AddReferencePathTracerTransportPass(
    FrameGraphBuilder& builder,
    const RenderFrame& frame,
    RenderViewportExtent extent,
    const RenderFrameGraphResources& resources,
    const ReferencePathTracerGraphResources& graphResources,
    const ReferencePathTracerWork& work,
    RenderRayTracingScene& rayTracingScene);

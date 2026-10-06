#pragma once

struct RenderFrame;

#include "Frame/Graph/RenderFrameGraphResources.h"
#include "Renderer/Public/Viewport/ViewportContracts.h"

class FrameGraphBuilder;
class GpuMeshCache;
class RenderRayTracingScene;

void AddGBufferPasses(
    FrameGraphBuilder& builder,
    const RenderFrame& frame,
    RenderViewportExtent sceneExtent,
    GpuMeshCache& gpuMeshCache,
    RenderRayTracingScene& rayTracingScene,
    RenderFrameGraphResources& resources);

#pragma once

#include "Frame/Graph/RenderFrameGraphResources.h"
#include "Renderer/Public/Viewport/ViewportContracts.h"

class FrameGraphBuilder;
class GpuMeshCache;
class RenderRayTracingScene;

void AddGBufferPasses(
    FrameGraphBuilder& builder,
    GpuMeshCache& gpuMeshCache,
    RenderRayTracingScene& rayTracingScene,
    RenderViewportExtent sceneExtent,
    RenderFrameGraphResources& resources);

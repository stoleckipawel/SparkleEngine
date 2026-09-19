#pragma once

#include "Frame/Graph/RenderFrameGraphResources.h"
#include "Renderer/Public/Viewport/ViewportContracts.h"

class FrameGraphBuilder;
class GpuMeshCache;
class RenderRayTracingScene;

void AddGBufferPasses(
    FrameGraphBuilder& builder,
    RenderViewportExtent sceneExtent,
    GpuMeshCache& gpuMeshCache,
    RenderRayTracingScene& rayTracingScene,
    RenderFrameGraphResources& resources);

#pragma once

#include "Frame/Graph/RenderFrameGraphResources.h"
#include "Frame/Graph/RenderFrameGraphTargets.h"
#include "Renderer/Public/Viewport/ViewportContracts.h"
#include "Renderer/Public/Settings/EngineRenderingRayTracingTypes.h"

class FrameGraphBuilder;
class GpuMeshCache;

GBufferRenderTargets CreateGBufferRenderTargets(FrameGraphBuilder& builder, RenderViewportExtent sceneExtent, GBufferMode gBufferMode);
void AddGBufferPasses(
    FrameGraphBuilder& builder,
    GpuMeshCache& gpuMeshCache,
    GBufferMode mode,
    RenderViewportExtent sceneExtent,
    RenderFrameGraphResources& resources);

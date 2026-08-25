#pragma once

#include "Frame/Graph/RenderFrameGraphResources.h"
#include "Frame/Graph/RenderFrameGraphTargets.h"
#include "Renderer/Public/Viewport/ViewportContracts.h"

class FrameGraphBuilder;
class GpuMeshCache;
class RenderRayTracingScene;

GBufferRenderTargets CreateGBufferRenderTargets(
	FrameGraphBuilder& builder,
	RenderViewportExtent sceneExtent);
void AddGBufferMeshPasses(
    FrameGraphBuilder& builder,
    GpuMeshCache& gpuMeshCache,
    const RenderRayTracingScene& rayTracingScene,
    bool hasMaskedRayTracingGeometry,
    RenderViewportExtent sceneExtent,
    RenderFrameGraphResources& resources);

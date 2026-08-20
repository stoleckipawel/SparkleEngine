#pragma once

#include "Frame/Core/FrameAssembly.h"
#include "Frame/Targets/FrameRenderTargets.h"
#include "Renderer/Public/Viewport/ViewportContracts.h"
#include "Renderer/Public/Settings/EngineRenderingRayTracingTypes.h"

class FrameGraphBuilder;
class GpuMeshCache;

GBufferRenderTargets CreateGBufferRenderTargets(FrameGraphBuilder& builder, RenderViewportExtent sceneExtent, GBufferMode gBufferMode);
void AddGBufferPasses(
    FrameGraphBuilder& builder,
    GpuMeshCache& gpuMeshCache,
    RenderViewportExtent sceneExtent,
    FrameAssemblyResourceLayout& resources);

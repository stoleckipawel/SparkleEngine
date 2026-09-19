#pragma once

#include "Renderer/Public/Viewport/RenderViewMode.h"

class FrameGraphBuilder;
class GpuMeshCache;
class ReferencePathTracerSession;
class RendererImageProviderStack;
class RenderRayTracingScene;
struct RenderFrameGraphSettings;
struct RenderFrameGraphResources;

void AddSceneRenderingPasses(
    FrameGraphBuilder& builder,
    const RenderFrameGraphSettings& settings,
    RenderViewMode viewMode,
    RenderRayTracingScene& rayTracingScene,
    GpuMeshCache& gpuMeshCache,
    RendererImageProviderStack& imageProviders,
    ReferencePathTracerSession& referencePathTracerSession,
    RenderFrameGraphResources& resources);

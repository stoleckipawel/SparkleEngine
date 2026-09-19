#pragma once

class FrameGraphBuilder;
class GpuMeshCache;
class RendererImageProviderStack;
class RenderRayTracingScene;
struct RenderFrameGraphResources;
struct RenderFrameGraphSettings;

void AddRealTimePathTracerPasses(
    FrameGraphBuilder& builder,
    const RenderFrameGraphSettings& settings,
    RenderRayTracingScene& rayTracingScene,
    GpuMeshCache& gpuMeshCache,
    RendererImageProviderStack& imageProviders,
    RenderFrameGraphResources& resources);

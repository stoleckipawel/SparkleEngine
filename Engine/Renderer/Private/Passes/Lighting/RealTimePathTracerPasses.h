#pragma once
#include <cstdint>

class FrameGraphBuilder;
class GpuMeshCache;
class RenderRayTracingScene;
struct RenderFrameGraphResources;
struct RenderFrameGraphSettings;

std::uint64_t GetRealTimePathTracerTopologyIdentity(const RenderFrameGraphSettings& settings) noexcept;

void AddRealTimePathTracerPasses(
    FrameGraphBuilder& builder,
    const RenderFrameGraphSettings& settings,
    RenderRayTracingScene& rayTracingScene,
    GpuMeshCache& gpuMeshCache,
    RenderFrameGraphResources& resources);

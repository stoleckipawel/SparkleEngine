#pragma once
#include "Renderer/Public/Viewport/ViewportContracts.h"
#include <cstdint>

class FrameGraphBuilder;
class GpuMeshCache;
class RenderRayTracingScene;
struct RenderFrameGraphResources;
struct RenderFrameGraphSettings;

std::uint64_t GetRealTimePathTracerTopologyIdentity() noexcept;

void AddRealTimePathTracerPasses(
    FrameGraphBuilder& builder,
    const RenderFrameGraphSettings& settings,
    RenderRayTracingScene& rayTracingScene,
    GpuMeshCache& gpuMeshCache,
    RenderFrameGraphResources& resources);

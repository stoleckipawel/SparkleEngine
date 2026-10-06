#pragma once

struct RenderFrame;
#include "Renderer/Public/Viewport/ViewportContracts.h"
#include <cstdint>

class FrameGraphBuilder;
class GpuMeshCache;
class RenderRayTracingScene;
struct RenderFrameGraphResources;
struct RenderFrameGraphSettings;

std::uint64_t GetRealTimePathTracerGraphRebuildKey() noexcept;

bool AddRealTimePathTracerPasses(
    FrameGraphBuilder& builder,
    const RenderFrame& frame,
    const RenderFrameGraphSettings& settings,
    RenderRayTracingScene& rayTracingScene,
    GpuMeshCache& gpuMeshCache,
    RenderFrameGraphResources& resources);

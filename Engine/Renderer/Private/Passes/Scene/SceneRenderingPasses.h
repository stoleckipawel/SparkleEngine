#pragma once

#include "Renderer/Public/Viewport/RenderViewMode.h"
#include <cstdint>

class FrameGraphBuilder;
class GpuMeshCache;
class ReferencePathTracerSession;
class RendererImageProviderStack;
class RenderRayTracingScene;
struct RenderFrameGraphSettings;
struct RenderFrameGraphResources;
struct ViewportFrameProducts;

std::uint64_t GetSceneRenderingTopologyIdentity(RenderViewMode viewMode, const RenderFrameGraphSettings& settings) noexcept;
bool PrepareSceneRenderingProducts(
    RenderViewMode viewMode,
    const RenderFrameGraphSettings& settings,
    const RenderFrameGraphResources& resources,
    ViewportFrameProducts& products) noexcept;

void AddSceneRenderingPasses(
    FrameGraphBuilder& builder,
    const RenderFrameGraphSettings& settings,
    RenderViewMode viewMode,
    RenderRayTracingScene& rayTracingScene,
    GpuMeshCache& gpuMeshCache,
    RendererImageProviderStack& imageProviders,
    ReferencePathTracerSession& referencePathTracerSession,
    RenderFrameGraphResources& resources);

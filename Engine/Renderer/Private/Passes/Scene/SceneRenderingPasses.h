#pragma once

#include "Renderer/Public/Viewport/RenderViewMode.h"
#include <cstdint>

struct RenderView;
struct ViewportRenderRequest;
class FrameGraphBuilder;
class GpuMeshCache;
class ReferencePathTracerSession;
class RendererImageProviderStack;
class RenderRayTracingScene;
struct RenderFrameGraphSettings;
struct RenderFrameGraphResources;
struct ViewportFrameProducts;

std::uint64_t GetSceneRenderingTopologyIdentity(RenderViewMode viewMode) noexcept;
bool PrepareSceneRenderingProducts(
    const RenderView& view,
    const RenderFrameGraphResources& resources,
    ViewportFrameProducts& products) noexcept;

void AddSceneRenderingPasses(
    FrameGraphBuilder& builder,
    const RenderFrameGraphSettings& settings,
    const ViewportRenderRequest& viewport,
    RenderRayTracingScene& rayTracingScene,
    GpuMeshCache& gpuMeshCache,
    RendererImageProviderStack& imageProviders,
    ReferencePathTracerSession& referencePathTracerSession,
    RenderFrameGraphResources& resources);

#pragma once

#include "Renderer/Public/Viewport/ViewportContracts.h"

#include <cstdint>

class FrameGraphBuilder;
class RenderRayTracingScene;
struct ReferencePathTracerGraphResources;
struct ReferencePathTracerUniformData;
struct RenderFrameGraphResources;

void AddReferencePathTracerGpuPasses(
    FrameGraphBuilder& builder,
    RenderViewportExtent extent,
    const RenderFrameGraphResources& resources,
    const ReferencePathTracerGraphResources& graphResources,
    const ReferencePathTracerUniformData& uniformData,
    std::uint32_t workRowsPerDispatch,
    RenderRayTracingScene& rayTracingScene);

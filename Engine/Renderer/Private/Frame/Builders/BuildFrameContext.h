#pragma once

#include "Renderer/Public/Viewport/ViewportContracts.h"

#include <cstdint>

class PerViewDataBuilder;
class RenderCamera;
class PersistentRenderGpuScene;
class TemporalDataBuilder;
class RenderSceneDataBuilder;
class RenderRayTracingScene;
struct FrameContext;
class RenderWorld;
struct RenderFrameDynamicData;

FrameContext BuildFrameContext(
    const RenderWorld& world,
    const RenderFrameDynamicData& dynamic,
    PersistentRenderGpuScene& gpuScene,
    std::uint32_t frameIndex,
    const RenderCamera& renderCamera,
    RenderViewportExtent sceneExtent,
    RenderSceneDataBuilder& renderSceneDataBuilder,
    RenderRayTracingScene* renderRayTracingScene,
    PerViewDataBuilder& perViewDataBuilder,
    TemporalDataBuilder& temporalDataBuilder);

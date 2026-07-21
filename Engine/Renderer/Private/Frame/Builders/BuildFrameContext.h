#pragma once

#include "Renderer/Public/Viewport/ViewportContracts.h"

class PerViewDataBuilder;
class RenderCamera;
class RhiResourceService;
class TemporalDataBuilder;
class RenderSceneDataBuilder;
class RenderRayTracingScene;
struct FrameContext;
class RenderWorld;
struct RenderFrameDynamicData;

FrameContext BuildFrameContext(
    const RenderWorld& world,
    const RenderFrameDynamicData& dynamic,
    RhiResourceService& resourceService,
    const RenderCamera& renderCamera,
    RenderViewportExtent sceneExtent,
    RenderSceneDataBuilder& renderSceneDataBuilder,
    RenderRayTracingScene* renderRayTracingScene,
    PerViewDataBuilder& perViewDataBuilder,
    TemporalDataBuilder& temporalDataBuilder);

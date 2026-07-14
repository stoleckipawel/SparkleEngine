#pragma once

#include "Renderer/Public/Viewport/ViewportContracts.h"

class PerViewDataBuilder;
class RenderCamera;
class RhiResourceService;
class TemporalDataBuilder;
class RenderSceneDataBuilder;
class RenderRayTracingScene;
struct FrameContext;
struct RenderSceneSnapshot;

FrameContext BuildFrameContext(
    const RenderSceneSnapshot& sceneSnapshot,
    RhiResourceService& resourceService,
    const RenderCamera& renderCamera,
    RenderViewportExtent sceneExtent,
    RenderSceneDataBuilder& renderSceneDataBuilder,
    RenderRayTracingScene* renderRayTracingScene,
    PerViewDataBuilder& perViewDataBuilder,
    TemporalDataBuilder& temporalDataBuilder);

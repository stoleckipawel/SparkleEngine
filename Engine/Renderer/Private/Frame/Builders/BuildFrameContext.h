#pragma once

#include "Renderer/Public/Viewport/ViewportContracts.h"

class PerViewDataBuilder;
class RenderCamera;
class RenderHardwareInterface;
class TemporalDataBuilder;
class RenderSceneDataBuilder;
class ViewLightingBuilder;
struct FrameContext;
struct RenderSceneSnapshot;

FrameContext BuildFrameContext(
    const RenderSceneSnapshot& sceneSnapshot,
    RenderHardwareInterface& renderHardwareInterface,
    const RenderCamera& renderCamera,
    RenderViewportExtent sceneExtent,
    RenderSceneDataBuilder& renderSceneDataBuilder,
    PerViewDataBuilder& perViewDataBuilder,
    ViewLightingBuilder& viewLightingBuilder,
    TemporalDataBuilder& temporalDataBuilder);

#pragma once

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
    RenderSceneDataBuilder& renderSceneDataBuilder,
    PerViewDataBuilder& perViewDataBuilder,
    ViewLightingBuilder& viewLightingBuilder,
    TemporalDataBuilder& temporalDataBuilder);

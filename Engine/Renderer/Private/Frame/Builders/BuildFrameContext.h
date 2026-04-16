#pragma once

class PerViewDataBuilder;
class RenderCamera;
class RenderHardwareInterface;
class RenderSceneDataBuilder;
class ShadowBuilder;
class ShadowFrameBuilder;
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
    ShadowFrameBuilder& shadowFrameBuilder,
    ShadowBuilder& shadowBuilder);

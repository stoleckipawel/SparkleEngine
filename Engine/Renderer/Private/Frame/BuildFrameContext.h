#pragma once

class D3D12SwapChain;
class D3D12ConstantBufferManager;
class PerViewDataBuilder;
class RenderCamera;
class RenderSceneDataBuilder;
class ShadowBuilder;
class ShadowFrameBuilder;
class ViewLightingBuilder;
struct FrameContext;
struct RenderSceneSnapshot;

FrameContext BuildFrameContext(
    const RenderSceneSnapshot& sceneSnapshot,
    const D3D12SwapChain& swapChain,
    D3D12ConstantBufferManager& constantBufferManager,
    const RenderCamera& renderCamera,
    RenderSceneDataBuilder& renderSceneDataBuilder,
	PerViewDataBuilder& perViewDataBuilder,
    ViewLightingBuilder& viewLightingBuilder,
    ShadowFrameBuilder& shadowFrameBuilder,
    ShadowBuilder& shadowBuilder);

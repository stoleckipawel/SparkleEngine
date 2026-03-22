#pragma once

#include "RHI/Public/D3D12/Resources/D3D12ViewLightingConstantBufferData.h"
#include "RHI/Public/D3D12/Resources/D3D12ShadowConstantBufferData.h"
#include "Renderer/Public/Frame/RenderViewContext.h"

#include <array>
#include <cstddef>

class D3D12ConstantBufferManager;
class PerViewDataBuilder;
class ShadowBuilder;
struct CameraSnapshot;
struct RenderSceneData;

struct ShadowFrameBuildResult
{
	static constexpr std::size_t MaxShadowedLights = RenderConfig::Lights::MaxDirectionalLights;

	PerViewLightingConstantBufferData mainViewLighting = {};
	std::array<RenderViewContext, MaxShadowedLights> shadowViews = {};
	std::size_t shadowViewCount = 0;
};

class ShadowFrameBuilder final
{
  public:
	ShadowFrameBuilder() noexcept = default;
	~ShadowFrameBuilder() noexcept = default;

	ShadowFrameBuilder(const ShadowFrameBuilder&) = delete;
	ShadowFrameBuilder& operator=(const ShadowFrameBuilder&) = delete;
	ShadowFrameBuilder(ShadowFrameBuilder&&) = delete;
	ShadowFrameBuilder& operator=(ShadowFrameBuilder&&) = delete;

	ShadowFrameBuildResult Build(
	    const CameraSnapshot& mainCamera,
	    const RenderSceneData& sceneData,
	    const PerViewLightingConstantBufferData& baseLighting,
	    D3D12ConstantBufferManager& constantBufferManager,
	    const PerViewDataBuilder& perViewDataBuilder,
	    ShadowBuilder& shadowBuilder) const;
};
#pragma once

#include "RHI/Public/D3D12/Resources/D3D12ViewLightingConstantBufferData.h"
#include "RHI/Public/D3D12/Resources/D3D12ShadowConstantBufferData.h"
#include "Renderer/Public/Frame/RenderViewContext.h"

class D3D12ConstantBufferManager;
class PerViewDataBuilder;
class ShadowBuilder;
struct CameraSnapshot;
struct RenderSceneData;

struct ShadowFrameBuildResult
{
	PerViewLightingConstantBufferData mainViewLighting = {};
	RenderViewContext shadowView = {};
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

  private:
	static PerViewLightingConstantBufferData BuildMainViewLighting(
	    const PerViewLightingConstantBufferData& baseLighting,
	    const ShadowConstantBufferData& shadowData) noexcept;
};
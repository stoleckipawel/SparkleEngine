#include "PCH.h"

#include "Frame/ShadowFrameBuilder.h"

#include "D3D12ConstantBufferManager.h"
#include "Frame/PerViewDataBuilder.h"
#include "Frame/ShadowBuilder.h"

PerViewLightingConstantBufferData ShadowFrameBuilder::BuildMainViewLighting(
	const PerViewLightingConstantBufferData& baseLighting,
	const ShadowConstantBufferData& shadowData) noexcept
{
	PerViewLightingConstantBufferData lighting = baseLighting;
	lighting.Shadow = shadowData;
	return lighting;
}

ShadowFrameBuildResult ShadowFrameBuilder::Build(
	const CameraSnapshot& mainCamera,
	const RenderSceneData& sceneData,
	const PerViewLightingConstantBufferData& baseLighting,
	D3D12ConstantBufferManager& constantBufferManager,
	const PerViewDataBuilder& perViewDataBuilder,
	ShadowBuilder& shadowBuilder) const
{
	ShadowFrameBuildResult result{};
	const ShadowBuildResult shadowData = shadowBuilder.Build(mainCamera, sceneData);
	result.mainViewLighting = BuildMainViewLighting(baseLighting, shadowData.shadow);

	RenderViewContext shadowView = perViewDataBuilder.BuildView(
	    shadowData.cameraData,
	    baseLighting,
	    shadowData.viewport,
	    shadowData.scissorRect);
	shadowView.perViewGpuAddress = constantBufferManager.AllocatePerView(shadowView.perViewData);
	result.shadowView = shadowView;

	return result;
}
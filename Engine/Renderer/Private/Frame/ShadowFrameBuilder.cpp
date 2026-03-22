#include "PCH.h"

#include "Frame/ShadowFrameBuilder.h"

#include "D3D12ConstantBufferManager.h"
#include "Frame/PerViewDataBuilder.h"
#include "Frame/ShadowBuilder.h"
#include "Renderer/Public/SceneData/RenderSceneData.h"

ShadowFrameBuildResult ShadowFrameBuilder::Build(
	const CameraSnapshot& mainCamera,
	const RenderSceneData& sceneData,
	const PerViewLightingConstantBufferData& baseLighting,
	D3D12ConstantBufferManager& constantBufferManager,
	const PerViewDataBuilder& perViewDataBuilder,
	ShadowBuilder& shadowBuilder) const
{
	ShadowFrameBuildResult result{};
	result.mainViewLighting = baseLighting;

	const std::size_t lightCount = std::min(
	    sceneData.directionalLights.size(),
	    ShadowFrameBuildResult::MaxShadowedLights);

	for (std::size_t i = 0; i < lightCount; ++i)
	{
		const ShadowBuildResult shadowData = shadowBuilder.Build(mainCamera, sceneData.directionalLights[i].direction);
		result.mainViewLighting.Shadows[i] = shadowData.shadow;

		RenderViewContext shadowView = perViewDataBuilder.BuildView(
		    shadowData.cameraData,
		    baseLighting,
		    shadowData.viewport,
		    shadowData.scissorRect);
		shadowView.perViewGpuAddress = constantBufferManager.AllocatePerView(shadowView.perViewData);
		result.shadowViews[i] = shadowView;
	}
	result.shadowViewCount = lightCount;

	return result;
}
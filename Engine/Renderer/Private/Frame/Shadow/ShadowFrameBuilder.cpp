#include "PCH.h"

#include "Frame/Shadow/ShadowFrameBuilder.h"

#include "D3D12/Resources/D3D12ConstantBufferManager.h"
#include "Frame/Builders/PerViewDataBuilder.h"
#include "Frame/Shadow/ShadowBuilder.h"
#include "Renderer/Public/SceneData/RenderSceneData.h"

std::array<ShadowFrameBuilder::CascadeRange, ShadowFrameBuildResult::MaxShadowCascades> ShadowFrameBuilder::BuildCascadeRanges(
    const CameraSnapshot& mainCamera) noexcept
{
	const float shadowDistance = std::min(mainCamera.farZ, RenderConfig::Shadows::ShadowDistance);
	const float nearCascadeFar = std::max(
	    mainCamera.nearZ + 0.001f,
	    mainCamera.nearZ + (shadowDistance - mainCamera.nearZ) * RenderConfig::Shadows::NearCascadeFraction);

	return {{
	    {mainCamera.nearZ, nearCascadeFar},
	    {nearCascadeFar, shadowDistance},
	}};
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
	result.mainViewLighting = baseLighting;

	const auto cascadeRanges = BuildCascadeRanges(mainCamera);
	const std::size_t lightCount = std::min(sceneData.directionalLights.size(), PerViewLightingConstantBufferData::MaxDirectionalLights);

	for (std::size_t lightIndex = 0; lightIndex < lightCount; ++lightIndex)
	{
		const std::size_t cascadeBaseIndex = lightIndex * ShadowFrameBuildResult::MaxShadowCascades;

		for (std::size_t cascadeIndex = 0; cascadeIndex < ShadowFrameBuildResult::MaxShadowCascades; ++cascadeIndex)
		{
			const std::size_t shadowIndex = cascadeBaseIndex + cascadeIndex;
			const CascadeRange& cascadeRange = cascadeRanges[cascadeIndex];
			const ShadowBuildResult shadowData =
			    shadowBuilder.Build(mainCamera, sceneData.directionalLights[lightIndex].direction, cascadeRange.nearZ, cascadeRange.farZ);
			result.mainViewLighting.Shadows[shadowIndex] = shadowData.shadow;

			RenderViewContext shadowView =
			    perViewDataBuilder.BuildView(shadowData.cameraData, baseLighting, shadowData.viewport, shadowData.scissorRect);
			shadowView.perViewGpuAddress = constantBufferManager.AllocatePerView(shadowView.perViewData);
			result.shadowViews[shadowIndex] = shadowView;
		}
	}

	result.shadowViewCount = lightCount * ShadowFrameBuildResult::MaxShadowCascades;
	return result;
}
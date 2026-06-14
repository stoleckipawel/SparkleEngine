#include "PCH.h"

#include "ViewLightingBuilder.h"

#include "RHI/Public/Resources/RenderViewLightingData.h"
#include "SceneData/RenderSceneData.h"

#include <algorithm>

PerViewLightingConstantBufferData ViewLightingBuilder::Build(const RenderSceneData& sceneData) const noexcept
{
	PerViewLightingConstantBufferData lighting{};

	const std::size_t directionalLightCount =
	    std::min(sceneData.directionalLights.size(), PerViewLightingConstantBufferData::MaxDirectionalLights);
	const std::size_t pointLightCount = std::min(sceneData.pointLights.size(), PerViewLightingConstantBufferData::MaxPointLights);
	const std::size_t spotLightCount = std::min(sceneData.spotLights.size(), PerViewLightingConstantBufferData::MaxSpotLights);
	lighting.DirectionalLightCount = static_cast<std::uint32_t>(directionalLightCount);
	lighting.PointLightCount = static_cast<std::uint32_t>(pointLightCount);
	lighting.SpotLightCount = static_cast<std::uint32_t>(spotLightCount);

	for (std::size_t lightIndex = 0; lightIndex < directionalLightCount; ++lightIndex)
	{
		const auto& sourceLight = sceneData.directionalLights[lightIndex];
		lighting.DirectionalLights[lightIndex].Direction = {sourceLight.direction.x, sourceLight.direction.y, sourceLight.direction.z};
		lighting.DirectionalLights[lightIndex].Intensity = sourceLight.intensity;
		lighting.DirectionalLights[lightIndex].Color = {sourceLight.color.x, sourceLight.color.y, sourceLight.color.z};
		lighting.DirectionalLights[lightIndex].AngularDiameter = sourceLight.angularDiameterRadians;
		lighting.DirectionalLights[lightIndex].CastShadow = sourceLight.castShadow ? 1u : 0u;
	}

	for (std::size_t lightIndex = 0; lightIndex < pointLightCount; ++lightIndex)
	{
		const auto& sourceLight = sceneData.pointLights[lightIndex];
		lighting.PointLights[lightIndex].Position = {sourceLight.position.x, sourceLight.position.y, sourceLight.position.z};
		lighting.PointLights[lightIndex].Range = sourceLight.range;
		lighting.PointLights[lightIndex].Color = {sourceLight.color.x, sourceLight.color.y, sourceLight.color.z};
		lighting.PointLights[lightIndex].Intensity = sourceLight.intensity;
		lighting.PointLights[lightIndex].SourceRadius = sourceLight.sourceRadius;
		lighting.PointLights[lightIndex].CastShadow = sourceLight.castShadow ? 1u : 0u;
	}

	for (std::size_t lightIndex = 0; lightIndex < spotLightCount; ++lightIndex)
	{
		const auto& sourceLight = sceneData.spotLights[lightIndex];
		lighting.SpotLights[lightIndex].Position = {sourceLight.position.x, sourceLight.position.y, sourceLight.position.z};
		lighting.SpotLights[lightIndex].Range = sourceLight.range;
		lighting.SpotLights[lightIndex].SourceRadius = sourceLight.sourceRadius;
		lighting.SpotLights[lightIndex].Direction = {sourceLight.direction.x, sourceLight.direction.y, sourceLight.direction.z};
		lighting.SpotLights[lightIndex].InnerConeCosine = sourceLight.innerConeCosine;
		lighting.SpotLights[lightIndex].Color = {sourceLight.color.x, sourceLight.color.y, sourceLight.color.z};
		lighting.SpotLights[lightIndex].Intensity = sourceLight.intensity;
		lighting.SpotLights[lightIndex].OuterConeCosine = sourceLight.outerConeCosine;
		lighting.SpotLights[lightIndex].CastShadow = sourceLight.castShadow ? 1u : 0u;
	}

	return lighting;
}

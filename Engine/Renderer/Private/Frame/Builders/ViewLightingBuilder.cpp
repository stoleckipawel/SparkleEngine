#include "PCH.h"

#include "ViewLightingBuilder.h"

#include "Renderer/Public/SceneData/RenderSceneData.h"

#include <algorithm>

PerViewLightingConstantBufferData ViewLightingBuilder::Build(const RenderSceneData& sceneData) const noexcept
{
	PerViewLightingConstantBufferData lighting{};

	const std::size_t directionalLightCount =
	    std::min(sceneData.directionalLights.size(), PerViewLightingConstantBufferData::MaxDirectionalLights);
	lighting.DirectionalLightCount = static_cast<std::uint32_t>(directionalLightCount);

	for (std::size_t lightIndex = 0; lightIndex < directionalLightCount; ++lightIndex)
	{
		const auto& sourceLight = sceneData.directionalLights[lightIndex];
		lighting.DirectionalLights[lightIndex].Direction = {sourceLight.direction.x, sourceLight.direction.y, sourceLight.direction.z};
		lighting.DirectionalLights[lightIndex].Intensity = sourceLight.intensity;
		lighting.DirectionalLights[lightIndex].Color = {sourceLight.color.x, sourceLight.color.y, sourceLight.color.z};
	}

	return lighting;
}
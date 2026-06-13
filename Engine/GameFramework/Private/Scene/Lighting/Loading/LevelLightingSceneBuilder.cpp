#include "PCH.h"
#include "Scene/Lighting/Loading/LevelLightingSceneBuilder.h"

#include "Scene/Lighting/SceneDirectionalLightDesc.h"

#include <string>
#include <utility>

namespace LevelLightingSceneBuilder
{
	std::vector<SceneLightDesc> BuildLights(const LevelLightingDesc& desc)
	{
		std::vector<SceneLightDesc> lights;
		lights.reserve(desc.directionalLights.size());

		for (std::size_t i = 0; i < desc.directionalLights.size(); ++i)
		{
			SceneLightDesc light;
			light.common.name = "Directional Light " + std::to_string(i + 1);
			light.common.color = desc.directionalLights[i].color;
			light.common.intensity = desc.directionalLights[i].intensity;
			SceneDirectionalLightDesc directional;
			directional.direction = desc.directionalLights[i].direction;
			directional.angularDiameterRadians = desc.directionalLights[i].angularDiameterRadians;
			directional.castShadow = desc.directionalLights[i].castShadow;
			light.payload = directional;
			lights.push_back(std::move(light));
		}

		return lights;
	}
}  // namespace LevelLightingSceneBuilder

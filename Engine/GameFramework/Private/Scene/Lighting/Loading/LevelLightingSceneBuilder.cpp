#include "PCH.h"
#include "Scene/Lighting/Loading/LevelLightingSceneBuilder.h"

#include "Config/RenderConfig.h"
#include "Scene/Lighting/SceneDirectionalLightDesc.h"

#include <string>
#include <utility>

namespace
{
	constexpr std::size_t kMaxDirectionalLights = RenderConfig::Lights::MaxDirectionalLights;
}  // namespace

namespace LevelLightingSceneBuilder
{
	std::vector<SceneLightDesc> BuildLights(const LevelLightingDesc& desc)
	{
		const std::size_t count = (std::min) (static_cast<std::size_t>(desc.directionalLightCount), kMaxDirectionalLights);
		std::vector<SceneLightDesc> lights;
		lights.reserve(count);

		for (std::size_t i = 0; i < count; ++i)
		{
			SceneLightDesc light;
			light.common.name = "Directional Light " + std::to_string(i + 1);
			light.common.color = desc.directionalLights[i].color;
			light.common.intensity = desc.directionalLights[i].intensity;
			SceneDirectionalLightDesc directional;
			directional.direction = desc.directionalLights[i].direction;
			directional.castShadow = desc.directionalLights[i].castShadow;
			light.payload = directional;
			lights.push_back(std::move(light));
		}

		return lights;
	}
}  // namespace LevelLightingSceneBuilder

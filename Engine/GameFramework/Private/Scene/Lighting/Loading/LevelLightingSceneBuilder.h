#pragma once

#include "Scene/Lighting/LevelLightingDesc.h"
#include "Scene/Lighting/SceneLightDesc.h"

#include <vector>

namespace LevelLightingSceneBuilder
{
	std::vector<SceneLightDesc> BuildLights(const LevelLightingDesc& desc);
}  // namespace LevelLightingSceneBuilder

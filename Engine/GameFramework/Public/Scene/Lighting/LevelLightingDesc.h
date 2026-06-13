#pragma once

#include "GameFramework/Public/GameFrameworkAPI.h"
#include "GameFramework/Public/Scene/Lighting/DirectionalLightDesc.h"

#include <vector>

struct SPARKLE_ENGINE_API LevelLightingDesc
{
	std::vector<DirectionalLightDesc> directionalLights;
};

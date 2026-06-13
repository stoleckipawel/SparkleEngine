#pragma once

#include "GameFramework/Public/GameFrameworkAPI.h"
#include "GameFramework/Public/Scene/Lighting/DirectionalLightDesc.h"
#include "GameFramework/Public/Scene/Lighting/PointLightSnapshotDesc.h"
#include "GameFramework/Public/Scene/Lighting/SpotLightSnapshotDesc.h"

#include <vector>

struct SPARKLE_ENGINE_API LightingSnapshot
{
	std::vector<DirectionalLightDesc> directionalLights;
	std::vector<PointLightSnapshotDesc> pointLights;
	std::vector<SpotLightSnapshotDesc> spotLights;
};

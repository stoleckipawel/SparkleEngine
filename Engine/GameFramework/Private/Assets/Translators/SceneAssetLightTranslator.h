#pragma once

#include "Assets/Cooked/CookedSceneLightRecord.h"
#include "Assets/SceneAssetPayload.h"

namespace Assets
{
	SceneLightDesc BuildSceneAssetLight(const CookedSceneLightRecord& lightRecord, std::size_t lightIndex);
}

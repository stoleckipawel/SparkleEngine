#pragma once

#include "Scene/Lighting/LightingSnapshot.h"
#include "Scene/Lighting/LevelLightingDesc.h"
#include "Scene/Lighting/SceneLightDesc.h"

#include <vector>

namespace SceneLightingSnapshotBuilder
{
	LevelLightingDesc BuildLevelDesc(const std::vector<SceneLightDesc>& lights) noexcept;
	LightingSnapshot BuildSnapshot(const std::vector<SceneLightDesc>& lights) noexcept;
}  // namespace SceneLightingSnapshotBuilder

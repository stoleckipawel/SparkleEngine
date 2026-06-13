#pragma once

#include "Scene/Lighting/LightingSnapshot.h"
#include "Scene/Lighting/SceneLightDesc.h"

#include <vector>

namespace SceneLightingSnapshotBuilder
{
	LightingSnapshot BuildSnapshot(const std::vector<SceneLightDesc>& lights) noexcept;
}  // namespace SceneLightingSnapshotBuilder

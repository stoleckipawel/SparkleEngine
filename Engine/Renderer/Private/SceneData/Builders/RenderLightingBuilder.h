#pragma once

#include "Rendering/RenderFrameDynamicData.h"

#include <vector>

struct RenderSceneData;

namespace RenderLightingBuilder
{
	void Build(const std::vector<RenderLightData>& lights, RenderSceneData& sceneData) noexcept;
}

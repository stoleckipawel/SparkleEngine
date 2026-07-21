#pragma once

#include "Scene/Lighting/SceneLightDesc.h"

#include <vector>

struct RenderSceneData;

namespace RenderLightingBuilder
{
	void Build(const std::vector<SceneLightDesc>& lights, RenderSceneData& sceneData) noexcept;
}

#pragma once

#include <cstddef>
#include <cstdint>

struct SceneLightingUniformData
{
	std::uint32_t DirectionalLightCount = 0;
	std::uint32_t PointLightCount = 0;
	std::uint32_t SpotLightCount = 0;
	std::uint32_t RectLightCount = 0;
};

static_assert(sizeof(SceneLightingUniformData) == 16, "Scene lighting constants must occupy one shader constant register");
static_assert(offsetof(SceneLightingUniformData, DirectionalLightCount) == 0);
static_assert(offsetof(SceneLightingUniformData, PointLightCount) == 4);
static_assert(offsetof(SceneLightingUniformData, SpotLightCount) == 8);
static_assert(offsetof(SceneLightingUniformData, RectLightCount) == 12);

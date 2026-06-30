#pragma once

#include <cstdint>

enum class SceneLightKind : std::uint32_t
{
	Directional = 0,
	Point = 1,
	Spot = 2,
	Rect = 3,
	Unknown = 4,
};

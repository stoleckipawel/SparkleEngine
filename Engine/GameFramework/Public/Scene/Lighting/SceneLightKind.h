#pragma once

#include <cstdint>

enum class SceneLightKind : std::uint32_t
{
	Directional = 0,
	Point = 1,
	Spot = 2,
	Unknown = 3,
};

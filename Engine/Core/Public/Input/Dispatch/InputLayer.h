#pragma once

#include <cstdint>

enum class InputLayer : std::uint8_t
{
	System = 0,
	Console = 1,
	Debug = 2,
	HUD = 3,
	Gameplay = 4,

	Count
};

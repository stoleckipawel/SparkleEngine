#pragma once

#include <cstdint>

enum class InputLayer : std::uint8_t
{
	System = 0,
	UI = 1,
	Gameplay = 2,

	Count
};

#pragma once

#include <cstdint>

enum class MouseButton : std::uint8_t
{
	Left = 0,
	Right = 1,
	Middle = 2,
	X1 = 3,
	X2 = 4,
	Button4 = X1,
	Button5 = X2,

	Count = 5
};

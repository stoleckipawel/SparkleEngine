#pragma once

#include <cstdint>

enum class InputDevice : std::uint8_t
{
	Keyboard = 0,
	Mouse,
	Gamepad,
	Touch,

	Count
};

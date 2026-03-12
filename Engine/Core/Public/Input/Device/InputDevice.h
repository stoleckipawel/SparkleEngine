#pragma once

#include <cstdint>

// =============================================================================
// InputDevice — Device Type Identifiers
// =============================================================================

enum class InputDevice : std::uint8_t
{
	Keyboard = 0,
	Mouse,
	Gamepad,
	Touch,

	Count
};

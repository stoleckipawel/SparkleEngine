#pragma once

#include <cstdint>

// =============================================================================
// MouseButton — Mouse Button Identifiers
// =============================================================================

enum class MouseButton : std::uint8_t
{
	Left = 0,
	Right = 1,
	Middle = 2,
	X1 = 3,        // XButton1 (back/side)
	X2 = 4,        // XButton2 (forward/side)
	Button4 = X1,  // Alias
	Button5 = X2,  // Alias

	Count = 5
};

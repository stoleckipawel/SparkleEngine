#pragma once

#include "../Mouse/MousePosition.h"
#include "../Keyboard/ModifierFlags.h"
#include "../Device/InputDevice.h"


// =============================================================================
// MouseMoveEvent
// =============================================================================

/// Event data for mouse cursor movement.
struct MouseMoveEvent
{
	MousePosition Position;                         ///< Current cursor position
	MouseDelta Delta;                               ///< Movement since last event
	ModifierFlags Modifiers = ModifierFlags::None;  ///< Active keyboard modifiers

	/// Returns the device type (always Mouse).
	constexpr InputDevice GetDevice() const noexcept { return InputDevice::Mouse; }

	/// Returns true if the specified modifier is active.
	constexpr bool HasModifier(ModifierFlags flag) const noexcept { return HasAnyFlag(Modifiers, flag); }
};

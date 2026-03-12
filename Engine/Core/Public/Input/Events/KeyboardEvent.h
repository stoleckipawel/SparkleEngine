// =============================================================================
// KeyboardEvent.h — Keyboard Input Event
// =============================================================================
//
// Event fired when a keyboard key is pressed or released.
//
// =============================================================================

#pragma once

#include "../Keyboard/Key.h"
#include "../Keyboard/ModifierFlags.h"
#include "../Device/InputDevice.h"

// =============================================================================
// KeyboardEvent
// =============================================================================

/// Event data for keyboard key press/release.
struct KeyboardEvent
{
	Key KeyCode = Key::Unknown;                     ///< Which key was pressed/released
	ModifierFlags Modifiers = ModifierFlags::None;  ///< Active modifiers at time of event
	bool bPressed = false;                          ///< True = press, False = release
	bool bRepeat = false;                           ///< True if OS auto-repeat

	/// Returns the device type (always Keyboard).
	constexpr InputDevice GetDevice() const noexcept { return InputDevice::Keyboard; }

	/// Returns true if this is a key press event.
	constexpr bool IsPressed() const noexcept { return bPressed; }

	/// Returns true if this is a key release event.
	constexpr bool IsReleased() const noexcept { return !bPressed; }

	/// Returns true if the specified modifier is active.
	constexpr bool HasModifier(ModifierFlags flag) const noexcept { return HasAnyFlag(Modifiers, flag); }
};

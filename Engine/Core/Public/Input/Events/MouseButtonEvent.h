// =============================================================================
// MouseButtonEvent.h — Mouse Button Input Event
// =============================================================================
//
// Event fired when a mouse button is pressed or released.
//
// =============================================================================

#pragma once

#include "../Mouse/MouseButton.h"
#include "../Mouse/MousePosition.h"
#include "../Keyboard/ModifierFlags.h"
#include "../Device/InputDevice.h"

// =============================================================================
// MouseButtonEvent
// =============================================================================

/// Event data for mouse button press/release.
struct MouseButtonEvent
{
	MouseButton Button = MouseButton::Left;         ///< Which button was pressed/released
	MousePosition Position;                         ///< Cursor position at time of event
	ModifierFlags Modifiers = ModifierFlags::None;  ///< Active keyboard modifiers
	bool bPressed = false;                          ///< True = press, False = release

	/// Returns the device type (always Mouse).
	constexpr InputDevice GetDevice() const noexcept { return InputDevice::Mouse; }

	/// Returns true if this is a button press event.
	constexpr bool IsPressed() const noexcept { return bPressed; }

	/// Returns true if this is a button release event.
	constexpr bool IsReleased() const noexcept { return !bPressed; }

	/// Returns true if the specified modifier is active.
	constexpr bool HasModifier(ModifierFlags flag) const noexcept { return HasAnyFlag(Modifiers, flag); }

	/// Returns true if this is a left button event.
	constexpr bool IsLeftButton() const noexcept { return Button == MouseButton::Left; }

	/// Returns true if this is a right button event.
	constexpr bool IsRightButton() const noexcept { return Button == MouseButton::Right; }

	/// Returns true if this is a middle button event.
	constexpr bool IsMiddleButton() const noexcept { return Button == MouseButton::Middle; }
};

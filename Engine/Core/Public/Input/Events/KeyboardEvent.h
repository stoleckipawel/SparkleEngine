#pragma once

#include "../Keyboard/Key.h"
#include "../Keyboard/ModifierFlags.h"
#include "../Device/InputDevice.h"

struct KeyboardEvent
{
	Key KeyCode = Key::Unknown;                     ///< Which key was pressed/released
	ModifierFlags Modifiers = ModifierFlags::None;  ///< Active modifiers at time of event
	bool bPressed = false;                          ///< True = press, False = release
	bool bRepeat = false;                           ///< True if OS auto-repeat

	constexpr InputDevice GetDevice() const noexcept { return InputDevice::Keyboard; }

	constexpr bool IsPressed() const noexcept { return bPressed; }

	constexpr bool IsReleased() const noexcept { return !bPressed; }

	constexpr bool HasModifier(ModifierFlags flag) const noexcept { return HasAnyFlag(Modifiers, flag); }
};

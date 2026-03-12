#pragma once

#include "../Keyboard/Key.h"
#include "../Keyboard/ModifierFlags.h"
#include "../Device/InputDevice.h"

struct KeyboardEvent
{
	Key KeyCode = Key::Unknown;
	ModifierFlags Modifiers = ModifierFlags::None;
	bool bPressed = false;
	bool bRepeat = false;

	constexpr InputDevice GetDevice() const noexcept { return InputDevice::Keyboard; }

	constexpr bool IsPressed() const noexcept { return bPressed; }

	constexpr bool IsReleased() const noexcept { return !bPressed; }

	constexpr bool HasModifier(ModifierFlags flag) const noexcept { return HasAnyFlag(Modifiers, flag); }
};

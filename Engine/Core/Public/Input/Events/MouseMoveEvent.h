#pragma once

#include "../Mouse/MousePosition.h"
#include "../Keyboard/ModifierFlags.h"
#include "../Device/InputDevice.h"

struct MouseMoveEvent
{
	MousePosition Position;
	MouseDelta Delta;
	ModifierFlags Modifiers = ModifierFlags::None;

	constexpr InputDevice GetDevice() const noexcept { return InputDevice::Mouse; }

	constexpr bool HasModifier(ModifierFlags flag) const noexcept { return HasAnyFlag(Modifiers, flag); }
};

#pragma once

#include "../Mouse/MousePosition.h"
#include "../Device/InputDevice.h"

struct MouseWheelEvent
{
	float Delta = 0.0f;
	MousePosition Position;
	bool bHorizontal = false;

	constexpr InputDevice GetDevice() const noexcept { return InputDevice::Mouse; }

	constexpr bool IsVertical() const noexcept { return !bHorizontal; }

	constexpr bool IsHorizontal() const noexcept { return bHorizontal; }

	constexpr bool IsPositive() const noexcept { return Delta > 0.0f; }

	constexpr bool IsNegative() const noexcept { return Delta < 0.0f; }
};

#pragma once

#include <cstdint>

enum class ButtonState : std::uint8_t
{
	Up = 0,
	Pressed = 1,
	Held = 2,
	Released = 3,
};

constexpr bool IsDown(ButtonState state) noexcept
{
	return state == ButtonState::Pressed || state == ButtonState::Held;
}

constexpr bool IsUp(ButtonState state) noexcept
{
	return state == ButtonState::Up || state == ButtonState::Released;
}

constexpr bool IsEdge(ButtonState state) noexcept
{
	return state == ButtonState::Pressed || state == ButtonState::Released;
}

constexpr bool IsContinuous(ButtonState state) noexcept
{
	return state == ButtonState::Up || state == ButtonState::Held;
}

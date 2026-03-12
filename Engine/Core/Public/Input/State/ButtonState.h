#pragma once

#include <cstdint>

// =============================================================================
// ButtonState — Four-State Button Status
// =============================================================================

enum class ButtonState : std::uint8_t
{
	Up = 0,        // Not pressed (continuous)
	Pressed = 1,   // Just pressed this frame (edge)
	Held = 2,      // Held down after first frame (continuous)
	Released = 3,  // Just released this frame (edge)
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

// =============================================================================
// ButtonState.h — Four-State Button Status
// =============================================================================
//
// Represents the current state of a key or button with precise per-frame
// edge detection. Used by both keyboard keys and mouse buttons.
//
// State transitions:
//   Up → Pressed → Held → Released → Up
//        (edge)   (cont)   (edge)   (cont)
//
// Frame behavior:
//   Frame N:   Button down → Pressed
//   Frame N+1: Button down → Held
//   Frame N+2: Button up   → Released
//   Frame N+3: Button up   → Up
//
// =============================================================================

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

// =============================================================================
// ButtonState Utilities
// =============================================================================

/// Returns true if the button is down (Pressed or Held).
constexpr bool IsDown(ButtonState state) noexcept
{
	return state == ButtonState::Pressed || state == ButtonState::Held;
}

/// Returns true if the button is up (Up or Released).
constexpr bool IsUp(ButtonState state) noexcept
{
	return state == ButtonState::Up || state == ButtonState::Released;
}

/// Returns true if this is an edge state (Pressed or Released).
constexpr bool IsEdge(ButtonState state) noexcept
{
	return state == ButtonState::Pressed || state == ButtonState::Released;
}

/// Returns true if this is a continuous state (Up or Held).
constexpr bool IsContinuous(ButtonState state) noexcept
{
	return state == ButtonState::Up || state == ButtonState::Held;
}

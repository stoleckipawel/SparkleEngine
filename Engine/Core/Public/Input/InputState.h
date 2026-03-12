// ============================================================================
// InputState.h
// ----------------------------------------------------------------------------
// Pollable input state container (single-threaded, main thread only).
//
// USAGE:
//   const InputState& state = inputSystem.GetState();
//   if (state.IsKeyPressed(Key::Space)) { Jump(); }
//   if (state.IsKeyHeld(Key::W)) { MoveForward(dt); }
//   auto delta = state.GetMouseDelta();
//
// DESIGN:
//   - Query-only public interface (mutations via friend InputSystem)
//   - Single-threaded: All access must be from main/game thread
//   - ButtonState 4-state model: Up → Pressed → Held → Released → Up
//   - Frame-edge detection: Pressed/Released valid for one frame only
//
// BUTTON STATE TRANSITIONS:
//   Frame N:   Key down detected     → State = Pressed
//   Frame N+1: Key still down        → State = Held
//   Frame M:   Key up detected       → State = Released
//   Frame M+1: Key still up          → State = Up
//
// THREADING:
//   This class is NOT thread-safe. All methods must be called from the
//   main thread. This matches industry practice (Unreal, Unity) where
//   input is processed and consumed on the game thread only.
//
// NOTES:
//   - IsKeyDown() returns true for both Pressed AND Held states
//   - IsKeyPressed() returns true only on the frame the key was pressed
//   - Mouse delta is accumulated between frames, cleared on BeginFrame()
//
// See also:
//   - InputSystem.h — Owns and updates InputState
//   - ButtonState.h — 4-state enum definition
//
// ============================================================================

#pragma once

#include "Core/Public/CoreAPI.h"

#include "Keyboard/Key.h"
#include "Keyboard/ModifierFlags.h"
#include "Mouse/MouseButton.h"
#include "Mouse/MousePosition.h"
#include "State/ButtonState.h"

#include <array>
#include <cstdint>

// Forward declaration for friend access
class InputSystem;

// ============================================================================
// InputState
// ============================================================================

/// Thread-safe container for pollable input state.
/// Updated by InputSystem, read by gameplay code.
class SPARKLE_CORE_API InputState
{
  public:
	InputState() = default;
	~InputState() = default;

	// Non-copyable (large state, explicit copy if needed)
	InputState(const InputState&) = delete;
	InputState& operator=(const InputState&) = delete;

	// Movable
	InputState(InputState&&) = default;
	InputState& operator=(InputState&&) = default;

	// =========================================================================
	// Keyboard Queries
	// =========================================================================

	/// Returns true if key is currently down (Pressed or Held).
	bool IsKeyDown(Key InKey) const noexcept;

	/// Returns true only on the frame the key was first pressed.
	bool IsKeyPressed(Key InKey) const noexcept;

	/// Returns true only on the frame the key was released.
	bool IsKeyReleased(Key InKey) const noexcept;

	/// Returns true if key has been held down for more than one frame.
	bool IsKeyHeld(Key InKey) const noexcept;

	/// Returns the raw ButtonState for a key.
	ButtonState GetKeyState(Key InKey) const noexcept;

	// =========================================================================
	// Mouse Button Queries
	// =========================================================================

	/// Returns true if mouse button is currently down (Pressed or Held).
	bool IsMouseButtonDown(MouseButton InButton) const noexcept;

	/// Returns true only on the frame the button was first pressed.
	bool IsMouseButtonPressed(MouseButton InButton) const noexcept;

	/// Returns true only on the frame the button was released.
	bool IsMouseButtonReleased(MouseButton InButton) const noexcept;

	/// Returns true if button has been held down for more than one frame.
	bool IsMouseButtonHeld(MouseButton InButton) const noexcept;

	/// Returns the raw ButtonState for a mouse button.
	ButtonState GetMouseButtonState(MouseButton InButton) const noexcept;

	// =========================================================================
	// Mouse Position Queries
	// =========================================================================

	/// Returns current mouse position in window coordinates (pixels).
	MousePosition GetMousePosition() const noexcept;

	/// Returns mouse movement delta since last frame (pixels).
	MousePosition GetMouseDelta() const noexcept;

	/// Returns accumulated mouse wheel delta since last frame.
	/// Positive = scroll up/forward, Negative = scroll down/backward.
	float GetMouseWheelDelta() const noexcept;

	/// Returns accumulated horizontal mouse wheel delta since last frame.
	/// Positive = scroll right, Negative = scroll left.
	float GetMouseWheelHorizontalDelta() const noexcept;

	// =========================================================================
	// Modifier Queries
	// =========================================================================

	/// Returns current modifier key flags (Shift, Ctrl, Alt, etc.).
	ModifierFlags GetModifiers() const noexcept;

	/// Returns true if the specified modifier is currently held.
	bool HasModifier(ModifierFlags InModifier) const noexcept;

	/// Convenience: Returns true if any Shift key is held.
	bool IsShiftDown() const noexcept;

	/// Convenience: Returns true if any Ctrl key is held.
	bool IsCtrlDown() const noexcept;

	/// Convenience: Returns true if any Alt key is held.
	bool IsAltDown() const noexcept;

	// =========================================================================
	// Mouse Capture Queries
	// =========================================================================

	/// Returns true if mouse is currently captured by the application.
	bool IsMouseCaptured() const noexcept;

	/// Returns true if cursor is currently hidden.
	bool IsCursorHidden() const noexcept;

  private:
	// =========================================================================
	// Friend Access (InputSystem manages state)
	// =========================================================================
	friend class InputSystem;

	// -------------------------------------------------------------------------
	// Mutation Methods (called by InputSystem only)
	// -------------------------------------------------------------------------

	/// Sets the state for a specific key.
	void SetKeyState(Key InKey, ButtonState InState) noexcept;

	/// Sets the state for a specific mouse button.
	void SetMouseButtonState(MouseButton InButton, ButtonState InState) noexcept;

	/// Sets the current mouse position.
	void SetMousePosition(int32_t X, int32_t Y) noexcept;

	/// Accumulates mouse movement delta (additive within frame).
	void AccumulateMouseDelta(int32_t DeltaX, int32_t DeltaY) noexcept;

	/// Accumulates vertical mouse wheel delta (additive within frame).
	void AccumulateWheelDelta(float Delta) noexcept;

	/// Accumulates horizontal mouse wheel delta (additive within frame).
	void AccumulateWheelHorizontalDelta(float Delta) noexcept;

	/// Sets the current modifier flags.
	void SetModifiers(ModifierFlags InModifiers) noexcept;

	/// Sets the mouse capture state.
	void SetMouseCaptured(bool bCaptured) noexcept;

	/// Sets the cursor hidden state.
	void SetCursorHidden(bool bHidden) noexcept;

	/// Called at the start of each frame to transition states.
	/// Pressed → Held, Released → Up, clears deltas.
	void BeginFrame() noexcept;

	/// Called at the end of each frame for optional cleanup.
	void EndFrame() noexcept;

	// -------------------------------------------------------------------------
	// Internal Storage
	// -------------------------------------------------------------------------

	/// Number of keys in the Key enum.
	static constexpr std::size_t KeyCount = static_cast<std::size_t>(Key::Count);

	/// Number of mouse buttons in the MouseButton enum.
	static constexpr std::size_t MouseButtonCount = static_cast<std::size_t>(MouseButton::Count);

	// Key states
	std::array<ButtonState, KeyCount> m_KeyStates{};

	// Mouse button states
	std::array<ButtonState, MouseButtonCount> m_MouseButtonStates{};

	// Mouse position
	int32_t m_MouseX = 0;
	int32_t m_MouseY = 0;

	// Mouse delta accumulated this frame
	int32_t m_MouseDeltaX = 0;
	int32_t m_MouseDeltaY = 0;

	// Mouse wheel delta accumulated this frame
	float m_WheelDelta = 0.0f;
	float m_WheelHorizontalDelta = 0.0f;

	// Current modifier flags
	ModifierFlags m_Modifiers = ModifierFlags::None;

	// Mouse capture/cursor state
	bool m_bMouseCaptured = false;
	bool m_bCursorHidden = false;
};

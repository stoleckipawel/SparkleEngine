#pragma once

#include "Core/Public/CoreAPI.h"

#include "Keyboard/Key.h"
#include "Keyboard/ModifierFlags.h"
#include "Mouse/MouseButton.h"
#include "Mouse/MousePosition.h"
#include "State/ButtonState.h"

#include <array>
#include <cstdint>

class InputSystem;

class SPARKLE_CORE_API InputState
{
  public:
	InputState() = default;
	~InputState() = default;

	InputState(const InputState&) = delete;
	InputState& operator=(const InputState&) = delete;

	InputState(InputState&&) = default;
	InputState& operator=(InputState&&) = default;

	bool IsKeyDown(Key InKey) const noexcept;

	bool IsKeyPressed(Key InKey) const noexcept;

	bool IsKeyReleased(Key InKey) const noexcept;

	bool IsKeyHeld(Key InKey) const noexcept;

	ButtonState GetKeyState(Key InKey) const noexcept;

	bool IsMouseButtonDown(MouseButton InButton) const noexcept;

	bool IsMouseButtonPressed(MouseButton InButton) const noexcept;

	bool IsMouseButtonReleased(MouseButton InButton) const noexcept;

	bool IsMouseButtonHeld(MouseButton InButton) const noexcept;

	ButtonState GetMouseButtonState(MouseButton InButton) const noexcept;

	MousePosition GetMousePosition() const noexcept;

	MousePosition GetMouseDelta() const noexcept;

	float GetMouseWheelDelta() const noexcept;

	float GetMouseWheelHorizontalDelta() const noexcept;

	ModifierFlags GetModifiers() const noexcept;

	bool HasModifier(ModifierFlags InModifier) const noexcept;

	bool IsShiftDown() const noexcept;

	bool IsCtrlDown() const noexcept;

	bool IsAltDown() const noexcept;

	bool IsMouseCaptured() const noexcept;

	bool IsCursorHidden() const noexcept;

  private:
	friend class InputSystem;

	void SetKeyState(Key InKey, ButtonState InState) noexcept;

	void SetMouseButtonState(MouseButton InButton, ButtonState InState) noexcept;

	void SetMousePosition(int32_t X, int32_t Y) noexcept;

	void AccumulateMouseDelta(int32_t DeltaX, int32_t DeltaY) noexcept;

	void AccumulateWheelDelta(float Delta) noexcept;

	void AccumulateWheelHorizontalDelta(float Delta) noexcept;

	void SetModifiers(ModifierFlags InModifiers) noexcept;

	void SetMouseCaptured(bool bCaptured) noexcept;

	void SetCursorHidden(bool bHidden) noexcept;

	void BeginFrame() noexcept;

	void EndFrame() noexcept;

	static constexpr std::size_t KeyCount = static_cast<std::size_t>(Key::Count);

	static constexpr std::size_t MouseButtonCount = static_cast<std::size_t>(MouseButton::Count);

	std::array<ButtonState, KeyCount> m_KeyStates{};

	std::array<ButtonState, MouseButtonCount> m_MouseButtonStates{};

	int32_t m_MouseX = 0;
	int32_t m_MouseY = 0;

	int32_t m_MouseDeltaX = 0;
	int32_t m_MouseDeltaY = 0;

	float m_WheelDelta = 0.0f;
	float m_WheelHorizontalDelta = 0.0f;

	ModifierFlags m_Modifiers = ModifierFlags::None;

	bool m_bMouseCaptured = false;
	bool m_bCursorHidden = false;
};

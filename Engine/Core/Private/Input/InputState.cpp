#include "PCH.h"
#include "InputState.h"

bool InputState::IsKeyDown(Key InKey) const noexcept
{
	const ButtonState state = GetKeyState(InKey);
	return state == ButtonState::Pressed || state == ButtonState::Held;
}

bool InputState::IsKeyPressed(Key InKey) const noexcept
{
	return GetKeyState(InKey) == ButtonState::Pressed;
}

bool InputState::IsKeyReleased(Key InKey) const noexcept
{
	return GetKeyState(InKey) == ButtonState::Released;
}

bool InputState::IsKeyHeld(Key InKey) const noexcept
{
	return GetKeyState(InKey) == ButtonState::Held;
}

ButtonState InputState::GetKeyState(Key InKey) const noexcept
{
	const std::size_t index = static_cast<std::size_t>(InKey);
	if (index >= KeyCount)
	{
		return ButtonState::Up;
	}
	return m_KeyStates[index];
}

bool InputState::IsMouseButtonDown(MouseButton InButton) const noexcept
{
	const ButtonState state = GetMouseButtonState(InButton);
	return state == ButtonState::Pressed || state == ButtonState::Held;
}

bool InputState::IsMouseButtonPressed(MouseButton InButton) const noexcept
{
	return GetMouseButtonState(InButton) == ButtonState::Pressed;
}

bool InputState::IsMouseButtonReleased(MouseButton InButton) const noexcept
{
	return GetMouseButtonState(InButton) == ButtonState::Released;
}

bool InputState::IsMouseButtonHeld(MouseButton InButton) const noexcept
{
	return GetMouseButtonState(InButton) == ButtonState::Held;
}

ButtonState InputState::GetMouseButtonState(MouseButton InButton) const noexcept
{
	const std::size_t index = static_cast<std::size_t>(InButton);
	if (index >= MouseButtonCount)
	{
		return ButtonState::Up;
	}
	return m_MouseButtonStates[index];
}

MousePosition InputState::GetMousePosition() const noexcept
{
	return MousePosition{m_MouseX, m_MouseY};
}

MousePosition InputState::GetMouseDelta() const noexcept
{
	return MousePosition{m_MouseDeltaX, m_MouseDeltaY};
}

float InputState::GetMouseWheelDelta() const noexcept
{
	return m_WheelDelta;
}

float InputState::GetMouseWheelHorizontalDelta() const noexcept
{
	return m_WheelHorizontalDelta;
}

ModifierFlags InputState::GetModifiers() const noexcept
{
	return m_Modifiers;
}

bool InputState::HasModifier(ModifierFlags InModifier) const noexcept
{
	return (static_cast<uint8_t>(m_Modifiers) & static_cast<uint8_t>(InModifier)) != 0;
}

bool InputState::IsShiftDown() const noexcept
{
	return HasModifier(ModifierFlags::Shift);
}

bool InputState::IsCtrlDown() const noexcept
{
	return HasModifier(ModifierFlags::Ctrl);
}

bool InputState::IsAltDown() const noexcept
{
	return HasModifier(ModifierFlags::Alt);
}

bool InputState::IsMouseCaptured() const noexcept
{
	return m_bMouseCaptured;
}

bool InputState::IsCursorHidden() const noexcept
{
	return m_bCursorHidden;
}

void InputState::SetKeyState(Key InKey, ButtonState InState) noexcept
{
	const std::size_t index = static_cast<std::size_t>(InKey);
	if (index < KeyCount)
	{
		m_KeyStates[index] = InState;
	}
}

void InputState::SetMouseButtonState(MouseButton InButton, ButtonState InState) noexcept
{
	const std::size_t index = static_cast<std::size_t>(InButton);
	if (index < MouseButtonCount)
	{
		m_MouseButtonStates[index] = InState;
	}
}

void InputState::SetMousePosition(int32_t X, int32_t Y) noexcept
{
	m_MouseX = X;
	m_MouseY = Y;
}

void InputState::AccumulateMouseDelta(int32_t DeltaX, int32_t DeltaY) noexcept
{
	m_MouseDeltaX += DeltaX;
	m_MouseDeltaY += DeltaY;
}

void InputState::AccumulateWheelDelta(float Delta) noexcept
{
	m_WheelDelta += Delta;
}

void InputState::AccumulateWheelHorizontalDelta(float Delta) noexcept
{
	m_WheelHorizontalDelta += Delta;
}

void InputState::SetModifiers(ModifierFlags InModifiers) noexcept
{
	m_Modifiers = InModifiers;
}

void InputState::SetMouseCaptured(bool bCaptured) noexcept
{
	m_bMouseCaptured = bCaptured;
}

void InputState::SetCursorHidden(bool bHidden) noexcept
{
	m_bCursorHidden = bHidden;
}

void InputState::BeginFrame() noexcept
{
	for (auto& state : m_KeyStates)
	{
		if (state == ButtonState::Pressed)
		{
			state = ButtonState::Held;
		}
		else if (state == ButtonState::Released)
		{
			state = ButtonState::Up;
		}
	}

	for (auto& state : m_MouseButtonStates)
	{
		if (state == ButtonState::Pressed)
		{
			state = ButtonState::Held;
		}
		else if (state == ButtonState::Released)
		{
			state = ButtonState::Up;
		}
	}

	m_MouseDeltaX = 0;
	m_MouseDeltaY = 0;
	m_WheelDelta = 0.0f;
	m_WheelHorizontalDelta = 0.0f;
}

void InputState::EndFrame() noexcept {}

#include "PCH.h"
#include "Win32InputBackend.h"

#include <windowsx.h>

static constexpr Key s_VirtualKeyToKey[] = {
    Key::Unknown,
    Key::Unknown,
    Key::Unknown,
    Key::Unknown,
    Key::Unknown,
    Key::Unknown,
    Key::Unknown,
    Key::Unknown,
    Key::Backspace,
    Key::Tab,
    Key::Unknown,
    Key::Unknown,
    Key::Unknown,
    Key::Enter,
    Key::Unknown,
    Key::Unknown,
    Key::LeftShift,
    Key::LeftCtrl,
    Key::LeftAlt,
    Key::Pause,
    Key::CapsLock,
    Key::Unknown,
    Key::Unknown,
    Key::Unknown,
    Key::Unknown,
    Key::Unknown,
    Key::Unknown,
    Key::Escape,
    Key::Unknown,
    Key::Unknown,
    Key::Unknown,
    Key::Unknown,
    Key::Space,
    Key::PageUp,
    Key::PageDown,
    Key::End,
    Key::Home,
    Key::Left,
    Key::Up,
    Key::Right,
    Key::Down,
    Key::Unknown,
    Key::Unknown,
    Key::Unknown,
    Key::PrintScreen,
    Key::Insert,
    Key::Delete,
    Key::Unknown,
    Key::Num0,
    Key::Num1,
    Key::Num2,
    Key::Num3,
    Key::Num4,
    Key::Num5,
    Key::Num6,
    Key::Num7,
    Key::Num8,
    Key::Num9,
    Key::Unknown,
    Key::Unknown,
    Key::Unknown,
    Key::Unknown,
    Key::Unknown,
    Key::Unknown,
    Key::Unknown,
    Key::A,
    Key::B,
    Key::C,
    Key::D,
    Key::E,
    Key::F,
    Key::G,
    Key::H,
    Key::I,
    Key::J,
    Key::K,
    Key::L,
    Key::M,
    Key::N,
    Key::O,
    Key::P,
    Key::Q,
    Key::R,
    Key::S,
    Key::T,
    Key::U,
    Key::V,
    Key::W,
    Key::X,
    Key::Y,
    Key::Z,
    Key::LeftSuper,
    Key::RightSuper,
    Key::Unknown,
    Key::Unknown,
    Key::Unknown,
    Key::Numpad0,
    Key::Numpad1,
    Key::Numpad2,
    Key::Numpad3,
    Key::Numpad4,
    Key::Numpad5,
    Key::Numpad6,
    Key::Numpad7,
    Key::Numpad8,
    Key::Numpad9,
    Key::NumpadMultiply,
    Key::NumpadAdd,
    Key::Unknown,
    Key::NumpadSubtract,
    Key::NumpadDecimal,
    Key::NumpadDivide,
    Key::F1,
    Key::F2,
    Key::F3,
    Key::F4,
    Key::F5,
    Key::F6,
    Key::F7,
    Key::F8,
    Key::F9,
    Key::F10,
    Key::F11,
    Key::F12,
};

static constexpr size_t s_VirtualKeyTableSize = sizeof(s_VirtualKeyToKey) / sizeof(s_VirtualKeyToKey[0]);

Key Win32InputBackend::TranslateVirtualKey(WPARAM VirtualKey) noexcept
{
	if (VirtualKey == VK_SHIFT)
	{
		if (GetKeyState(VK_LSHIFT) & 0x8000)
			return Key::LeftShift;
		if (GetKeyState(VK_RSHIFT) & 0x8000)
			return Key::RightShift;
		return Key::LeftShift;
	}
	if (VirtualKey == VK_CONTROL)
	{
		if (GetKeyState(VK_LCONTROL) & 0x8000)
			return Key::LeftCtrl;
		if (GetKeyState(VK_RCONTROL) & 0x8000)
			return Key::RightCtrl;
		return Key::LeftCtrl;
	}
	if (VirtualKey == VK_MENU)
	{
		if (GetKeyState(VK_LMENU) & 0x8000)
			return Key::LeftAlt;
		if (GetKeyState(VK_RMENU) & 0x8000)
			return Key::RightAlt;
		return Key::LeftAlt;
	}

	if (VirtualKey == VK_LSHIFT)
		return Key::LeftShift;
	if (VirtualKey == VK_RSHIFT)
		return Key::RightShift;
	if (VirtualKey == VK_LCONTROL)
		return Key::LeftCtrl;
	if (VirtualKey == VK_RCONTROL)
		return Key::RightCtrl;
	if (VirtualKey == VK_LMENU)
		return Key::LeftAlt;
	if (VirtualKey == VK_RMENU)
		return Key::RightAlt;

	if (VirtualKey == VK_RETURN)
	{
		return Key::Enter;
	}

	if (VirtualKey < s_VirtualKeyTableSize)
	{
		return s_VirtualKeyToKey[VirtualKey];
	}

	switch (VirtualKey)
	{
		case VK_OEM_1:
			return Key::Semicolon;
		case VK_OEM_PLUS:
			return Key::Equals;
		case VK_OEM_COMMA:
			return Key::Comma;
		case VK_OEM_MINUS:
			return Key::Minus;
		case VK_OEM_PERIOD:
			return Key::Period;
		case VK_OEM_2:
			return Key::Slash;
		case VK_OEM_3:
			return Key::Grave;
		case VK_OEM_4:
			return Key::LeftBracket;
		case VK_OEM_5:
			return Key::Backslash;
		case VK_OEM_6:
			return Key::RightBracket;
		case VK_OEM_7:
			return Key::Apostrophe;
		case VK_NUMLOCK:
			return Key::NumLock;
		case VK_SCROLL:
			return Key::ScrollLock;
		default:
			return Key::Unknown;
	}
}

ModifierFlags Win32InputBackend::GetCurrentModifiers() noexcept
{
	ModifierFlags Flags = ModifierFlags::None;

	if (GetKeyState(VK_LSHIFT) & 0x8000)
		Flags = Flags | ModifierFlags::LeftShift;
	if (GetKeyState(VK_RSHIFT) & 0x8000)
		Flags = Flags | ModifierFlags::RightShift;
	if (GetKeyState(VK_LCONTROL) & 0x8000)
		Flags = Flags | ModifierFlags::LeftCtrl;
	if (GetKeyState(VK_RCONTROL) & 0x8000)
		Flags = Flags | ModifierFlags::RightCtrl;
	if (GetKeyState(VK_LMENU) & 0x8000)
		Flags = Flags | ModifierFlags::LeftAlt;
	if (GetKeyState(VK_RMENU) & 0x8000)
		Flags = Flags | ModifierFlags::RightAlt;
	if (GetKeyState(VK_CAPITAL) & 0x0001)
		Flags = Flags | ModifierFlags::CapsLock;
	if (GetKeyState(VK_NUMLOCK) & 0x0001)
		Flags = Flags | ModifierFlags::NumLock;

	return Flags;
}

InputBackendResult Win32InputBackend::ProcessMessage(uint32_t Msg, uintptr_t Param1, intptr_t Param2)
{
	InputBackendResult Result;
	WPARAM WParam = static_cast<WPARAM>(Param1);
	LPARAM LParam = static_cast<LPARAM>(Param2);

	switch (Msg)
	{
		case WM_KEYDOWN:
		case WM_SYSKEYDOWN:
		{
			Result.Type = InputEventType::Keyboard;
			Result.Keyboard.KeyCode = TranslateVirtualKey(WParam);
			Result.Keyboard.bPressed = true;
			Result.Keyboard.bRepeat = (LParam & 0x40000000) != 0;
			Result.Keyboard.Modifiers = GetCurrentModifiers();
			break;
		}

		case WM_KEYUP:
		case WM_SYSKEYUP:
		{
			Result.Type = InputEventType::Keyboard;
			Result.Keyboard.KeyCode = TranslateVirtualKey(WParam);
			Result.Keyboard.bPressed = false;
			Result.Keyboard.bRepeat = false;
			Result.Keyboard.Modifiers = GetCurrentModifiers();
			break;
		}

		case WM_LBUTTONDOWN:
		{
			Result.Type = InputEventType::MouseButton;
			Result.MouseButton.Button = MouseButton::Left;
			Result.MouseButton.bPressed = true;
			Result.MouseButton.Position.X = GET_X_LPARAM(LParam);
			Result.MouseButton.Position.Y = GET_Y_LPARAM(LParam);
			Result.MouseButton.Modifiers = GetCurrentModifiers();
			break;
		}

		case WM_LBUTTONUP:
		{
			Result.Type = InputEventType::MouseButton;
			Result.MouseButton.Button = MouseButton::Left;
			Result.MouseButton.bPressed = false;
			Result.MouseButton.Position.X = GET_X_LPARAM(LParam);
			Result.MouseButton.Position.Y = GET_Y_LPARAM(LParam);
			Result.MouseButton.Modifiers = GetCurrentModifiers();
			break;
		}

		case WM_RBUTTONDOWN:
		{
			Result.Type = InputEventType::MouseButton;
			Result.MouseButton.Button = MouseButton::Right;
			Result.MouseButton.bPressed = true;
			Result.MouseButton.Position.X = GET_X_LPARAM(LParam);
			Result.MouseButton.Position.Y = GET_Y_LPARAM(LParam);
			Result.MouseButton.Modifiers = GetCurrentModifiers();
			break;
		}

		case WM_RBUTTONUP:
		{
			Result.Type = InputEventType::MouseButton;
			Result.MouseButton.Button = MouseButton::Right;
			Result.MouseButton.bPressed = false;
			Result.MouseButton.Position.X = GET_X_LPARAM(LParam);
			Result.MouseButton.Position.Y = GET_Y_LPARAM(LParam);
			Result.MouseButton.Modifiers = GetCurrentModifiers();
			break;
		}

		case WM_MBUTTONDOWN:
		{
			Result.Type = InputEventType::MouseButton;
			Result.MouseButton.Button = MouseButton::Middle;
			Result.MouseButton.bPressed = true;
			Result.MouseButton.Position.X = GET_X_LPARAM(LParam);
			Result.MouseButton.Position.Y = GET_Y_LPARAM(LParam);
			Result.MouseButton.Modifiers = GetCurrentModifiers();
			break;
		}

		case WM_MBUTTONUP:
		{
			Result.Type = InputEventType::MouseButton;
			Result.MouseButton.Button = MouseButton::Middle;
			Result.MouseButton.bPressed = false;
			Result.MouseButton.Position.X = GET_X_LPARAM(LParam);
			Result.MouseButton.Position.Y = GET_Y_LPARAM(LParam);
			Result.MouseButton.Modifiers = GetCurrentModifiers();
			break;
		}

		case WM_XBUTTONDOWN:
		{
			Result.Type = InputEventType::MouseButton;
			Result.MouseButton.Button = (GET_XBUTTON_WPARAM(WParam) == XBUTTON1) ? MouseButton::X1 : MouseButton::X2;
			Result.MouseButton.bPressed = true;
			Result.MouseButton.Position.X = GET_X_LPARAM(LParam);
			Result.MouseButton.Position.Y = GET_Y_LPARAM(LParam);
			Result.MouseButton.Modifiers = GetCurrentModifiers();
			break;
		}

		case WM_XBUTTONUP:
		{
			Result.Type = InputEventType::MouseButton;
			Result.MouseButton.Button = (GET_XBUTTON_WPARAM(WParam) == XBUTTON1) ? MouseButton::X1 : MouseButton::X2;
			Result.MouseButton.bPressed = false;
			Result.MouseButton.Position.X = GET_X_LPARAM(LParam);
			Result.MouseButton.Position.Y = GET_Y_LPARAM(LParam);
			Result.MouseButton.Modifiers = GetCurrentModifiers();
			break;
		}

		case WM_MOUSEMOVE:
		{
			Result.Type = InputEventType::MouseMove;
			Result.MouseMove.Position.X = GET_X_LPARAM(LParam);
			Result.MouseMove.Position.Y = GET_Y_LPARAM(LParam);

			Result.MouseMove.Delta.X = 0;
			Result.MouseMove.Delta.Y = 0;
			break;
		}

		case WM_MOUSEWHEEL:
		{
			Result.Type = InputEventType::MouseWheel;
			Result.MouseWheel.Delta = static_cast<float>(GET_WHEEL_DELTA_WPARAM(WParam)) / WHEEL_DELTA;
			Result.MouseWheel.bHorizontal = false;
			Result.MouseWheel.Position.X = GET_X_LPARAM(LParam);
			Result.MouseWheel.Position.Y = GET_Y_LPARAM(LParam);
			break;
		}

		case WM_MOUSEHWHEEL:
		{
			Result.Type = InputEventType::MouseWheel;
			Result.MouseWheel.Delta = static_cast<float>(GET_WHEEL_DELTA_WPARAM(WParam)) / WHEEL_DELTA;
			Result.MouseWheel.bHorizontal = true;
			Result.MouseWheel.Position.X = GET_X_LPARAM(LParam);
			Result.MouseWheel.Position.Y = GET_Y_LPARAM(LParam);
			break;
		}

		default:

			break;
	}

	return Result;
}

#pragma once

#include <cstdint>

enum class Key : std::uint16_t
{
	Unknown = 0,

	A = 1,
	B,
	C,
	D,
	E,
	F,
	G,
	H,
	I,
	J,
	K,
	L,
	M,
	N,
	O,
	P,
	Q,
	R,
	S,
	T,
	U,
	V,
	W,
	X,
	Y,
	Z,

	Num0 = 27,
	Num1,
	Num2,
	Num3,
	Num4,
	Num5,
	Num6,
	Num7,
	Num8,
	Num9,

	F1 = 37,
	F2,
	F3,
	F4,
	F5,
	F6,
	F7,
	F8,
	F9,
	F10,
	F11,
	F12,

	Up = 49,
	Down,
	Left,
	Right,
	Home,
	End,
	PageUp,
	PageDown,
	Insert,
	Delete,

	LeftShift = 59,
	RightShift,
	LeftCtrl,
	RightCtrl,
	LeftAlt,
	RightAlt,
	LeftSuper,
	RightSuper,

	Space = 67,
	Enter,
	Escape,
	Tab,
	Backspace,
	CapsLock,
	NumLock,
	ScrollLock,
	PrintScreen,
	Pause,

	Comma = 77,
	Period,
	Slash,
	Semicolon,
	Apostrophe,
	LeftBracket,
	RightBracket,
	Backslash,
	Grave,
	Minus,
	Equals,

	Numpad0 = 88,
	Numpad1,
	Numpad2,
	Numpad3,
	Numpad4,
	Numpad5,
	Numpad6,
	Numpad7,
	Numpad8,
	Numpad9,
	NumpadDecimal,
	NumpadEnter,
	NumpadAdd,
	NumpadSubtract,
	NumpadMultiply,
	NumpadDivide,

	Count
};

constexpr bool IsLetterKey(Key key) noexcept
{
	return key >= Key::A && key <= Key::Z;
}

constexpr bool IsNumberKey(Key key) noexcept
{
	return key >= Key::Num0 && key <= Key::Num9;
}

constexpr bool IsFunctionKey(Key key) noexcept
{
	return key >= Key::F1 && key <= Key::F12;
}

constexpr bool IsNumpadKey(Key key) noexcept
{
	return key >= Key::Numpad0 && key <= Key::NumpadDivide;
}

constexpr bool IsModifierKey(Key key) noexcept
{
	return key >= Key::LeftShift && key <= Key::RightSuper;
}

constexpr bool IsNavigationKey(Key key) noexcept
{
	return key >= Key::Up && key <= Key::Delete;
}

// =============================================================================
// Key.h — Platform-Agnostic Keyboard Key Codes
// =============================================================================
//
// Portable key codes with zero-based values. Platform backends translate
// native codes (VK_*, XKB, etc.) to these values.
//
// =============================================================================

#pragma once

#include <cstdint>

// =============================================================================
// Key — Platform-Agnostic Key Codes
// =============================================================================

enum class Key : std::uint16_t
{
	Unknown = 0,

	// -------------------------------------------------------------------------
	// Alphabet (1-26)
	// -------------------------------------------------------------------------
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

	// -------------------------------------------------------------------------
	// Number Row (27-36)
	// -------------------------------------------------------------------------
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

	// -------------------------------------------------------------------------
	// Function Keys (37-48)
	// -------------------------------------------------------------------------
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

	// -------------------------------------------------------------------------
	// Navigation (49-58)
	// -------------------------------------------------------------------------
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

	// -------------------------------------------------------------------------
	// Modifier Keys (59-66)
	// -------------------------------------------------------------------------
	LeftShift = 59,
	RightShift,
	LeftCtrl,
	RightCtrl,
	LeftAlt,
	RightAlt,
	LeftSuper,  // Windows key / Command
	RightSuper,

	// -------------------------------------------------------------------------
	// Common Keys (67-76)
	// -------------------------------------------------------------------------
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

	// -------------------------------------------------------------------------
	// Punctuation / Symbols (77-87)
	// -------------------------------------------------------------------------
	Comma = 77,    // ,<
	Period,        // .>
	Slash,         // /?
	Semicolon,     // ;:
	Apostrophe,    // '"
	LeftBracket,   // [{
	RightBracket,  // ]}
	Backslash,     // \|
	Grave,         // `~
	Minus,         // -_
	Equals,        // =+

	// -------------------------------------------------------------------------
	// Numpad (88-103)
	// -------------------------------------------------------------------------
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

	// -------------------------------------------------------------------------
	// Count marker for array sizing
	// -------------------------------------------------------------------------
	Count
};

// =============================================================================
// Key Classification Utilities
// =============================================================================

/// Returns true if this is a letter key (A-Z).
constexpr bool IsLetterKey(Key key) noexcept
{
	return key >= Key::A && key <= Key::Z;
}

/// Returns true if this is a number key (0-9 on number row).
constexpr bool IsNumberKey(Key key) noexcept
{
	return key >= Key::Num0 && key <= Key::Num9;
}

/// Returns true if this is a function key (F1-F12).
constexpr bool IsFunctionKey(Key key) noexcept
{
	return key >= Key::F1 && key <= Key::F12;
}

/// Returns true if this is a numpad key.
constexpr bool IsNumpadKey(Key key) noexcept
{
	return key >= Key::Numpad0 && key <= Key::NumpadDivide;
}

/// Returns true if this is a modifier key (Shift, Ctrl, Alt, Super).
constexpr bool IsModifierKey(Key key) noexcept
{
	return key >= Key::LeftShift && key <= Key::RightSuper;
}

/// Returns true if this is an arrow/navigation key.
constexpr bool IsNavigationKey(Key key) noexcept
{
	return key >= Key::Up && key <= Key::Delete;
}

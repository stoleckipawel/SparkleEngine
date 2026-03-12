#pragma once

#include <cstdint>
#include <type_traits>

// =============================================================================
// ModifierFlags — Keyboard Modifier Bitmask
// =============================================================================

enum class ModifierFlags : std::uint16_t
{
	None = 0,

	// Individual keys
	LeftShift = 1 << 0,
	RightShift = 1 << 1,
	LeftCtrl = 1 << 2,
	RightCtrl = 1 << 3,
	LeftAlt = 1 << 4,
	RightAlt = 1 << 5,
	LeftSuper = 1 << 6,
	RightSuper = 1 << 7,

	// Toggle states
	CapsLock = 1 << 8,
	NumLock = 1 << 9,
	ScrollLock = 1 << 10,

	// Combined (either left or right)
	Shift = LeftShift | RightShift,
	Ctrl = LeftCtrl | RightCtrl,
	Alt = LeftAlt | RightAlt,
	Super = LeftSuper | RightSuper,

	// Common combinations
	CtrlShift = Ctrl | Shift,
	CtrlAlt = Ctrl | Alt,
	ShiftAlt = Shift | Alt,
};

// =============================================================================
// Bitwise Operators
// =============================================================================

constexpr ModifierFlags operator|(ModifierFlags lhs, ModifierFlags rhs) noexcept
{
	return static_cast<ModifierFlags>(
	    static_cast<std::underlying_type_t<ModifierFlags>>(lhs) | static_cast<std::underlying_type_t<ModifierFlags>>(rhs));
}

constexpr ModifierFlags operator&(ModifierFlags lhs, ModifierFlags rhs) noexcept
{
	return static_cast<ModifierFlags>(
	    static_cast<std::underlying_type_t<ModifierFlags>>(lhs) & static_cast<std::underlying_type_t<ModifierFlags>>(rhs));
}

constexpr ModifierFlags operator^(ModifierFlags lhs, ModifierFlags rhs) noexcept
{
	return static_cast<ModifierFlags>(
	    static_cast<std::underlying_type_t<ModifierFlags>>(lhs) ^ static_cast<std::underlying_type_t<ModifierFlags>>(rhs));
}

constexpr ModifierFlags operator~(ModifierFlags flags) noexcept
{
	return static_cast<ModifierFlags>(~static_cast<std::underlying_type_t<ModifierFlags>>(flags));
}

constexpr ModifierFlags& operator|=(ModifierFlags& lhs, ModifierFlags rhs) noexcept
{
	lhs = lhs | rhs;
	return lhs;
}

constexpr ModifierFlags& operator&=(ModifierFlags& lhs, ModifierFlags rhs) noexcept
{
	lhs = lhs & rhs;
	return lhs;
}

constexpr ModifierFlags& operator^=(ModifierFlags& lhs, ModifierFlags rhs) noexcept
{
	lhs = lhs ^ rhs;
	return lhs;
}

// =============================================================================
// ModifierFlags Utilities
// =============================================================================

/// Checks if all specified modifier flags are set.
constexpr bool HasAllFlags(ModifierFlags flags, ModifierFlags test) noexcept
{
	return (flags & test) == test;
}

/// Checks if any of the specified modifier flags are set.
constexpr bool HasAnyFlag(ModifierFlags flags, ModifierFlags test) noexcept
{
	return (flags & test) != ModifierFlags::None;
}

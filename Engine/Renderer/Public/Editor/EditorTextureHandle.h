#pragma once

#include <compare>
#include <cstdint>

struct EditorTextureHandle final
{
	std::uint32_t Slot = 0;
	std::uint32_t Generation = 0;

	constexpr explicit operator bool() const noexcept
	{
		return Slot != 0 && Generation != 0;
	}

	constexpr std::uint64_t Pack() const noexcept
	{
		return (static_cast<std::uint64_t>(Generation) << 32u) | Slot;
	}

	static constexpr EditorTextureHandle Unpack(std::uint64_t value) noexcept
	{
		return EditorTextureHandle{
		    .Slot = static_cast<std::uint32_t>(value),
		    .Generation = static_cast<std::uint32_t>(value >> 32u)};
	}

	constexpr auto operator<=>(const EditorTextureHandle&) const noexcept = default;
};

#pragma once

#include <compare>
#include <cstdint>
#include <limits>

struct MaterialHandle
{
	std::uint32_t index = INVALID_INDEX;

	static constexpr std::uint32_t INVALID_INDEX = (std::numeric_limits<std::uint32_t>::max)();

	constexpr MaterialHandle() noexcept = default;
	explicit constexpr MaterialHandle(std::uint32_t slot) noexcept : index(slot) {}

	static constexpr MaterialHandle Invalid() noexcept { return MaterialHandle{}; }

	constexpr bool IsValid() const noexcept { return index != INVALID_INDEX; }

	constexpr std::uint32_t GetIndex() const noexcept { return index; }

	constexpr auto operator<=>(const MaterialHandle&) const noexcept = default;
};
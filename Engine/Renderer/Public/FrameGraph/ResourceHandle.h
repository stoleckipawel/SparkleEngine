#pragma once

#include <compare>
#include <cstdint>
#include <limits>

struct ResourceHandle
{
	std::uint32_t index = std::numeric_limits<std::uint32_t>::max();

	static constexpr std::uint32_t INVALID_INDEX = std::numeric_limits<std::uint32_t>::max();

	static constexpr ResourceHandle Invalid() noexcept { return ResourceHandle{INVALID_INDEX}; }

	constexpr bool IsValid() const noexcept { return index != INVALID_INDEX; }

	constexpr auto operator<=>(const ResourceHandle&) const noexcept = default;
};

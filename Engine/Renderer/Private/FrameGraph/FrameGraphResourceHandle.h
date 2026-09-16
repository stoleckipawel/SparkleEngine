#pragma once

#include <compare>
#include <cstdint>
#include <limits>

struct FrameGraphResourceHandle
{
	std::uint32_t index = (std::numeric_limits<std::uint32_t>::max)();

	static constexpr std::uint32_t INVALID_INDEX = (std::numeric_limits<std::uint32_t>::max)();

	static constexpr FrameGraphResourceHandle Invalid() noexcept { return FrameGraphResourceHandle{INVALID_INDEX}; }

	constexpr bool IsValid() const noexcept { return index != INVALID_INDEX; }

	constexpr auto operator<=>(const FrameGraphResourceHandle&) const noexcept = default;
};

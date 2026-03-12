#pragma once

#include <compare>
#include <cstdint>
#include <limits>

struct ResourceHandle
{
	std::uint32_t index = std::numeric_limits<std::uint32_t>::max();

	static constexpr std::uint32_t INVALID_INDEX = std::numeric_limits<std::uint32_t>::max();

	static constexpr std::uint32_t BACKBUFFER_INDEX = 0;

	static constexpr std::uint32_t DEPTH_BUFFER_INDEX = 1;

	static constexpr ResourceHandle Invalid() noexcept { return ResourceHandle{INVALID_INDEX}; }

	static constexpr ResourceHandle BackBuffer() noexcept { return ResourceHandle{BACKBUFFER_INDEX}; }

	static constexpr ResourceHandle DepthBuffer() noexcept { return ResourceHandle{DEPTH_BUFFER_INDEX}; }

	constexpr bool IsValid() const noexcept { return index != INVALID_INDEX; }

	constexpr bool IsBackBuffer() const noexcept { return index == BACKBUFFER_INDEX; }

	constexpr bool IsDepthBuffer() const noexcept { return index == DEPTH_BUFFER_INDEX; }

	constexpr auto operator<=>(const ResourceHandle&) const noexcept = default;
};

#pragma once

#include <compare>
#include <cstdint>
#include <limits>

struct MaterialHandle
{
	std::uint32_t index = INVALID_INDEX;
	std::uint32_t generation = 0;

	static constexpr std::uint32_t INVALID_INDEX = (std::numeric_limits<std::uint32_t>::max)();

	constexpr MaterialHandle() noexcept = default;
	explicit constexpr MaterialHandle(std::uint32_t slot, std::uint32_t resourceGeneration = 0) noexcept :
	    index(slot), generation(resourceGeneration)
	{
	}

	static constexpr MaterialHandle Invalid() noexcept { return MaterialHandle{}; }

	constexpr bool IsValid() const noexcept { return index != INVALID_INDEX; }

	constexpr std::uint32_t GetIndex() const noexcept { return index; }
	constexpr std::uint32_t GetGeneration() const noexcept { return generation; }

	constexpr auto operator<=>(const MaterialHandle&) const noexcept = default;
};

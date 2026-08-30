#pragma once

#include <cstdint>
#include <limits>

template <typename Tag> struct RhiGenerationalHandle
{
	std::uint32_t Value = 0;

	static constexpr std::uint32_t IndexBitCount = 16u;
	static constexpr std::uint32_t IndexMask = (1u << IndexBitCount) - 1u;
	static constexpr std::uint32_t MaximumRecordCount = IndexMask;
	static constexpr std::uint16_t MaximumGeneration = std::numeric_limits<std::uint16_t>::max();

	static constexpr RhiGenerationalHandle Make(std::uint32_t index, std::uint16_t generation) noexcept
	{
		return index < MaximumRecordCount ? RhiGenerationalHandle{(static_cast<std::uint32_t>(generation) << IndexBitCount) | (index + 1u)}
		                                  : RhiGenerationalHandle{};
	}

	constexpr bool Decode(std::uint32_t& outIndex, std::uint16_t& outGeneration) const noexcept
	{
		const std::uint32_t encodedIndex = Value & IndexMask;
		if (encodedIndex == 0)
		{
			return false;
		}

		outIndex = encodedIndex - 1u;
		outGeneration = static_cast<std::uint16_t>(Value >> IndexBitCount);
		return true;
	}

	constexpr explicit operator bool() const noexcept { return Value != 0; }
	constexpr bool operator==(const RhiGenerationalHandle&) const noexcept = default;
};

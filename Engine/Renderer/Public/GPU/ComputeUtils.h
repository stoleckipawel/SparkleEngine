#pragma once

#include <cassert>
#include <cstdint>

namespace ComputeUtils
{
	inline std::uint32_t DivideRoundUp(std::uint32_t value, std::uint32_t divisor) noexcept
	{
		assert(divisor > 0);
		return (value + divisor - 1) / divisor;
	}
}  // namespace ComputeUtils
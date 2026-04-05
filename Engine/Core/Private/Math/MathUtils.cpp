#include "PCH.h"

#include "Core/Public/Math/MathUtils.h"

#include <cassert>

namespace MathUtils
{
	std::uint32_t DivideRoundUp(std::uint32_t value, std::uint32_t divisor) noexcept
	{
		assert(divisor > 0);
		return (value + divisor - 1) / divisor;
	}

	std::uint64_t AlignUp(std::uint64_t value, std::uint64_t alignment) noexcept
	{
		assert(alignment > 0);
		return (value + alignment - 1) & ~(alignment - 1);
	}
}
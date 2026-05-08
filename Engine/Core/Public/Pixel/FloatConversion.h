#pragma once

#include <cstdint>

namespace Pixel
{
	// IEEE 754 float32 to float16 (half precision) conversion
	// Handles special values (NaN, Inf, denormals) correctly
	std::uint16_t FloatToHalf(float value) noexcept;
}

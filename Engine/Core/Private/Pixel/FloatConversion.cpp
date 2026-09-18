#include "PCH.h"

#include "Core/Public/Pixel/FloatConversion.h"

#include <bit>
#include <cstring>

namespace Pixel
{
	std::uint16_t FloatToHalf(float value) noexcept
	{
		// Bit-cast float to uint32
		std::uint32_t f32bits;
		std::memcpy(&f32bits, &value, sizeof(float));

		// Extract components
		std::uint32_t sign = (f32bits >> 31) & 0x1;
		std::int32_t exponent = static_cast<std::int32_t>((f32bits >> 23) & 0xFF) - 127;
		std::uint32_t mantissa = f32bits & 0x7FFFFF;

		// Out-of-range exponent maps to Inf/0
		if (exponent > 15)
		{
			return static_cast<std::uint16_t>((sign << 15) | 0x7C00); // Inf
		}
		if (exponent < -24)
		{
			return static_cast<std::uint16_t>(sign << 15); // 0
		}

		// Subnormal handling
		if (exponent < -14)
		{
			const int shift = 23 - (exponent + 24);
			const std::uint32_t bits = ((mantissa | 0x800000) >> shift) & 0x3FF;
			return static_cast<std::uint16_t>((sign << 15) | bits);
		}

		// Normal case
		std::uint16_t h16_exp = static_cast<std::uint16_t>((exponent + 15) & 0x1F);
		std::uint16_t h16_frac = static_cast<std::uint16_t>((mantissa >> 13) & 0x3FF);

		return static_cast<std::uint16_t>((sign << 15) | (h16_exp << 10) | h16_frac);
	}

	float HalfToFloat(std::uint16_t value) noexcept
	{
		const std::uint32_t sign = static_cast<std::uint32_t>(value & 0x8000u) << 16u;
		std::int32_t exponent = static_cast<std::int32_t>((value >> 10u) & 0x1fu);
		std::uint32_t mantissa = value & 0x03ffu;
		std::uint32_t bits = 0u;
		if (exponent == 0)
		{
			if (mantissa == 0u)
			{
				bits = sign;
			}
			else
			{
				while ((mantissa & 0x0400u) == 0u)
				{
					mantissa <<= 1u;
					--exponent;
				}
				mantissa &= 0x03ffu;
				bits = sign | (static_cast<std::uint32_t>(exponent + 113) << 23u) | (mantissa << 13u);
			}
		}
		else if (exponent == 0x1f)
		{
			bits = sign | 0x7f800000u | (mantissa << 13u);
		}
		else
		{
			bits = sign | (static_cast<std::uint32_t>(exponent + 112) << 23u) | (mantissa << 13u);
		}
		return std::bit_cast<float>(bits);
	}
}

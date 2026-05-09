#include "PCH.h"

#include "Core/Public/Math/SignalProcessing.h"

#include <cmath>
#include <numbers>

namespace MathUtils
{
	float BesselI0(float x) noexcept
	{
		x = std::fabs(x);
		if (x < 3.75f)
		{
			const float y = (x / 3.75f) * (x / 3.75f);
			return 1.0f + y * (3.5156229f + y * (3.0899424f + y * (1.2067492f + y * (0.2659732f + y * (0.0360768f + y * 0.0045564f)))));
		}

		const float y = 3.75f / x;
		return (std::exp(x) / std::sqrt(x)) *
		       (0.39894228f +
		        y * (0.01328592f +
		             y * (0.00225319f +
		                  y * (-0.00157565f +
		                       y * (0.00916281f + y * (-0.02057706f + y * (0.02635537f + y * (-0.01647633f + y * 0.00392377f))))))));
	}

	float Sinc(float x) noexcept
	{
		if (std::fabs(x) < 1e-8f)
		{
			return 1.0f;
		}
		const float px = std::numbers::pi_v<float> * x;
		return std::sin(px) / px;
	}

	float KaiserKernel(float x, float beta, void* reserved) noexcept
	{
		(void) reserved;

		const float bx = beta * std::sqrt(1.0f - (x * x));
		if (bx > 40.0f)
		{
			return 1.0f;
		}
		if (bx < -40.0f)
		{
			return 0.0f;
		}

		const float i0_beta = BesselI0(beta);
		return i0_beta > 0.0f ? BesselI0(bx) / i0_beta : 1.0f;
	}

	float KaiserSupport(float beta) noexcept
	{
		return 1.0f;
	}
}

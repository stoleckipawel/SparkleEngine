#pragma once

namespace MathUtils
{
	// Modified Bessel function of the first kind, order 0
	// Used in Kaiser window calculations for windowed-sinc filtering
	float BesselI0(float x) noexcept;

	// Sinc function: sin(πx) / (πx), with special handling for x near 0
	float Sinc(float x) noexcept;

	// Kaiser window kernel
	// Parameters:
	//   x - sample position (typically in range [-support, support])
	//   beta - shape parameter (controls window characteristics)
	//   reserved - reserved for future use (pass nullptr)
	// Returns: window coefficient at position x
	float KaiserKernel(float x, float beta, void* reserved) noexcept;

	// Kaiser window support (half-width before envelope reaches zero)
	float KaiserSupport(float beta) noexcept;
}

#pragma once

namespace CommonRandom
{
	static const float InterleavedGradientNoiseFrameStep = 5.588238f;

	uint Hash(uint value)
	{
		value ^= 2747636419u;
		value *= 2654435769u;
		value ^= value >> 16u;
		value *= 2654435769u;
		value ^= value >> 16u;
		value *= 2654435769u;
		return value;
	}

	float Random01(inout uint state)
	{
		state = Hash(state);
		return (float)(state & 0x00FFFFFFu) / 16777216.0f;
	}

	float2 Random02(inout uint state)
	{
		return float2(Random01(state), Random01(state));
	}

	uint2 MultiplyHighLow(uint lhs, uint rhs)
	{
		const uint lhsLow = lhs & 0xFFFFu;
		const uint lhsHigh = lhs >> 16u;
		const uint rhsLow = rhs & 0xFFFFu;
		const uint rhsHigh = rhs >> 16u;
		const uint lowProduct = lhsLow * rhsLow;
		const uint cross = lhsHigh * rhsLow + (lowProduct >> 16u);
		const uint crossLow = (cross & 0xFFFFu) + lhsLow * rhsHigh;
		const uint high = lhsHigh * rhsHigh + (cross >> 16u) + (crossLow >> 16u);
		const uint low = (crossLow << 16u) | (lowProduct & 0xFFFFu);
		return uint2(high, low);
	}

	uint4 Philox4x32Round(uint4 counter, uint2 key)
	{
		const uint2 product0 = MultiplyHighLow(0xD2511F53u, counter.x);
		const uint2 product1 = MultiplyHighLow(0xCD9E8D57u, counter.z);
		return uint4(product1.x ^ counter.y ^ key.x, product1.y, product0.x ^ counter.w ^ key.y, product0.y);
	}

	uint4 Philox4x32(uint4 counter, uint2 key)
	{
		[unroll] for (uint roundIndex = 0u; roundIndex < 10u; ++roundIndex)
		{
			counter = Philox4x32Round(counter, key);
			key += uint2(0x9E3779B9u, 0xBB67AE85u);
		}
		return counter;
	}

	float OpenUnitInterval(uint word)
	{
		return ((float)(word >> 8u) + 0.5f) * 0x1.0p-24f;
	}

	float InterleavedGradientNoise(float2 pixelCoord, uint frameIndex, float2 offset)
	{
		const float frameOffset = InterleavedGradientNoiseFrameStep * (float)(frameIndex & 63u);
		const float2 animatedPixel = pixelCoord + offset + frameOffset.xx;
		const float gradient = frac(dot(animatedPixel, float2(0.06711056f, 0.00583715f)));
		return frac(52.9829189f * gradient);
	}

	float2 InterleavedGradientNoise2(float2 pixelCoord, uint frameIndex, float2 offset)
	{
		return float2(InterleavedGradientNoise(pixelCoord, frameIndex, offset),
		              InterleavedGradientNoise(pixelCoord, frameIndex, offset + float2(19.19f, 73.73f)));
	}
}

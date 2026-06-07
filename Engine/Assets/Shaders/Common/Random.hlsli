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

	float InterleavedGradientNoise(float2 pixelCoord, uint frameIndex, float2 offset)
	{
		const float frameOffset = InterleavedGradientNoiseFrameStep * (float)(frameIndex & 63u);
		const float2 animatedPixel = pixelCoord + offset + frameOffset.xx;
		const float gradient = frac(dot(animatedPixel, float2(0.06711056f, 0.00583715f)));
		return frac(52.9829189f * gradient);
	}

	float2 InterleavedGradientNoise2(float2 pixelCoord, uint frameIndex, float2 offset)
	{
		return float2(
		    InterleavedGradientNoise(pixelCoord, frameIndex, offset),
		    InterleavedGradientNoise(pixelCoord, frameIndex, offset + float2(19.19f, 73.73f)));
	}
}

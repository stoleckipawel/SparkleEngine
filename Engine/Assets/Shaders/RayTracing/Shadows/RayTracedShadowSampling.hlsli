#ifndef SPARKLE_RAY_TRACED_SHADOW_SAMPLING_HLSLI
#define SPARKLE_RAY_TRACED_SHADOW_SAMPLING_HLSLI

#include "/Engine/Resources/FrameUniformData.hlsli"

#include "/Engine/Common/Random.hlsli"

namespace RayTracedShadowSampling
{
	float2 BuildAnimatedSample(uint2 pixelCoord, uint lightIndex, uint dimensionTag)
	{
		const float2 lightOffset = float2((float)(lightIndex * 17u + dimensionTag * 131u), (float)(lightIndex * 59u + dimensionTag * 7u));
		return CommonRandom::InterleavedGradientNoise2(float2(pixelCoord), FrameIndex, lightOffset);
	}
}

#endif

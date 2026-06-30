#ifndef SPARKLE_RAY_TRACED_SHADOW_SAMPLING_HLSLI
#define SPARKLE_RAY_TRACED_SHADOW_SAMPLING_HLSLI

#include "Common/Random.hlsli"
#include "Common/Sampling.hlsli"

namespace RayTracedShadowSampling
{
	float2 BuildAnimatedSample(uint2 pixelCoord, uint lightIndex, uint dimensionTag)
	{
		const float2 lightOffset = float2(
		    (float)(lightIndex * 17u + dimensionTag * 131u),
		    (float)(lightIndex * 59u + dimensionTag * 7u));
		return CommonRandom::InterleavedGradientNoise2(float2(pixelCoord), FrameIndex, lightOffset);
	}

	float3 SampleConeDirection(float3 axis, float coneHalfAngleRadians, float2 sample)
	{
		return CommonSampling::SampleConeDirection(axis, coneHalfAngleRadians, sample);
	}

	float3 SampleSpherePoint(float3 center, float radius, float3 referenceDirection, float2 sample)
	{
		return CommonSampling::SampleSpherePoint(center, radius, referenceDirection, sample);
	}

	float3 SampleDiskPoint(float3 center, float3 normal, float radius, float2 sample)
	{
		return CommonSampling::SampleDiskPoint(center, normal, radius, sample);
	}
}

#endif

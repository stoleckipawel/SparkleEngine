#ifndef SPARKLE_RESTIR_RESERVOIR_COMMON_HLSLI
#define SPARKLE_RESTIR_RESERVOIR_COMMON_HLSLI

#include "/Engine/Resources/FrameUniformData.hlsli"

#include "/Engine/Common/Random.hlsli"
namespace RestirReservoirCommon
{
	static const uint InitialCandidateCount = 4u;
	static const uint SpatialNeighborCount = 4u;
	static const float MaxTemporalM = 20.0f;
	static const float MaxSpatialM = 32.0f;
	static const float NormalSimilarityThreshold = 0.85f;
	static const float RelativeDepthThreshold = 0.05f;
	static const float MinimumDepthThreshold = 0.10f;

	uint BuildSeed(uint2 pixelCoord, uint tag)
	{
		uint seed = pixelCoord.x * 1973u;
		seed ^= pixelCoord.y * 9277u;
		seed ^= FrameIndex * 26699u;
		seed ^= tag * 911u;
		return CommonRandom::Hash(seed);
	}

	float4 PackSurface(bool valid, float3 normalWorld, float viewDistance)
	{
		return valid ? float4(normalWorld, viewDistance) : 0.0f.xxxx;
	}

	bool IsPackedSurfaceValid(float4 packedSurface)
	{
		return packedSurface.w > 0.0f && dot(packedSurface.xyz, packedSurface.xyz) > 0.25f;
	}

	bool AreSurfacesCompatible(bool valid, float3 normalWorld, float viewDistance, float4 packedSurface)
	{
		if (!valid || !IsPackedSurfaceValid(packedSurface))
		{
			return false;
		}

		const float normalSimilarity = dot(normalWorld, normalize(packedSurface.xyz));
		const float depthThreshold = max(MinimumDepthThreshold, viewDistance * RelativeDepthThreshold);
		return normalSimilarity >= NormalSimilarityThreshold && abs(viewDistance - packedSurface.w) <= depthThreshold;
	}
}

#endif

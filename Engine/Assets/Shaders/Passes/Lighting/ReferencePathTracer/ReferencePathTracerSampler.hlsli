#ifndef SPARKLE_REFERENCE_PATH_TRACER_SAMPLER_HLSLI
#define SPARKLE_REFERENCE_PATH_TRACER_SAMPLER_HLSLI

#include "/Engine/Common/Random.hlsli"

namespace ReferencePathTracerSampler
{
	static const uint FilmX = 0u;
	static const uint FilmY = 1u;
	static const uint SurfaceDimensionBegin = 8u;
	static const uint SurfaceDimensionStride = 8u;
	static const uint BsdfDirectionXOffset = 4u;
	static const uint BsdfDirectionYOffset = 5u;
	static const uint MaximumDimension = 32775u;
	static const uint MaximumPixelCoordinate = 16383u;
	static const uint MaximumSampleOrdinal = 1048575u;

	bool IsValidIdentity(uint2 pixelCoord, uint sampleOrdinal, uint dimensionId)
	{
		return all(pixelCoord <= MaximumPixelCoordinate) && sampleOrdinal <= MaximumSampleOrdinal && dimensionId <= MaximumDimension;
	}

	uint Word(uint2 pixelCoord, uint sampleOrdinal, uint dimensionId, uint sessionSeed, uint replicateId)
	{
		const uint dimensionBlock = dimensionId >> 2u;
		const uint packedPixel = (pixelCoord.y << 14u) | pixelCoord.x;
		const uint4 result =
		    CommonRandom::Philox4x32(uint4(packedPixel, sampleOrdinal, dimensionBlock, 0x52505431u), uint2(sessionSeed, replicateId));
		return result[dimensionId & 3u];
	}

	uint SurfaceDimension(uint surfaceDepth, uint offset)
	{
		return SurfaceDimensionBegin + SurfaceDimensionStride * surfaceDepth + offset;
	}

}

#endif

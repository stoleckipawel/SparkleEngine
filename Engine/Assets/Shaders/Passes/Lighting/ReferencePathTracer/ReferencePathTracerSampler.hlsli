#ifndef SPARKLE_REFERENCE_PATH_TRACER_SAMPLER_HLSLI
#define SPARKLE_REFERENCE_PATH_TRACER_SAMPLER_HLSLI

#include "/Engine/Common/Random.hlsli"

namespace ReferencePathTracer
{
	static const uint FilmXDimension = 0u;
	static const uint FilmYDimension = 1u;
	static const uint SurfaceDimensionBegin = 8u;
	static const uint SurfaceDimensionStride = 8u;
	static const uint LightChoiceOffset = 0u;
	static const uint LightShapeXOffset = 1u;
	static const uint LightShapeYOffset = 2u;
	static const uint LobeChoiceOffset = 3u;
	static const uint BsdfDirectionXOffset = 4u;
	static const uint BsdfDirectionYOffset = 5u;
	static const uint RouletteOffset = 6u;

	struct SampleIdentity
	{
		uint2 PixelCoord;
		uint SampleOrdinal;
		uint SessionSeed;
		uint ReplicateId;
	};

	uint RandomWord(SampleIdentity identity, uint dimensionId)
	{
		const uint dimensionBlock = dimensionId >> 2u;
		const uint packedPixel = (identity.PixelCoord.y << 14u) | identity.PixelCoord.x;
		const uint4 result = CommonRandom::Philox4x32(uint4(packedPixel, identity.SampleOrdinal, dimensionBlock, 0x52505431u),
		                                              uint2(identity.SessionSeed, identity.ReplicateId));
		return result[dimensionId & 3u];
	}

	uint SurfaceDimension(uint surfaceDepth, uint offset)
	{
		return SurfaceDimensionBegin + SurfaceDimensionStride * surfaceDepth + offset;
	}
}

#endif

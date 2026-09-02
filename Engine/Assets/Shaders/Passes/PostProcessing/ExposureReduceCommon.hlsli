#ifndef SPARKLE_EXPOSURE_REDUCE_COMMON_HLSLI
#define SPARKLE_EXPOSURE_REDUCE_COMMON_HLSLI

RWTexture2D<float4> LuminanceMomentsOutput;
groupshared float2 ExposureReduceSharedMoments[256];

namespace ExposureReduce
{
	void StoreGroup(uint linearThreadIndex, uint2 outputPixel, float2 moments)
	{
		ExposureReduceSharedMoments[linearThreadIndex] = moments;
		GroupMemoryBarrierWithGroupSync();

		[unroll]
		for (uint stride = 128u; stride > 0u; stride >>= 1u)
		{
			if (linearThreadIndex < stride)
			{
				ExposureReduceSharedMoments[linearThreadIndex] += ExposureReduceSharedMoments[linearThreadIndex + stride];
			}
			GroupMemoryBarrierWithGroupSync();
		}

		if (linearThreadIndex == 0u)
		{
			LuminanceMomentsOutput[outputPixel] = float4(ExposureReduceSharedMoments[0], 0.0f, 0.0f);
		}
	}
}

#endif

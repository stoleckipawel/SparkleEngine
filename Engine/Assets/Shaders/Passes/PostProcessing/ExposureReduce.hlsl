#include "Display/Exposure.hlsli"

Texture2D SceneColor;
Texture2D LuminanceMomentsInput;
RWTexture2D<float4> LuminanceMomentsOutput;

groupshared float2 SharedMoments[256];

void StoreReducedGroup(uint linearThreadIndex, uint2 outputPixel, float2 moments)
{
	SharedMoments[linearThreadIndex] = moments;
	GroupMemoryBarrierWithGroupSync();

	[unroll] for (uint stride = 128u; stride > 0u; stride >>= 1u)
	{
		if (linearThreadIndex < stride)
		{
			SharedMoments[linearThreadIndex] += SharedMoments[linearThreadIndex + stride];
		}
		GroupMemoryBarrierWithGroupSync();
	}

	if (linearThreadIndex == 0u)
	{
		LuminanceMomentsOutput[outputPixel] = float4(SharedMoments[0], 0.0f, 0.0f);
	}
}

[numthreads(16, 16, 1)] void ReduceSceneMain(uint3 groupId : SV_GroupID, uint3 groupThreadId : SV_GroupThreadID)
{
	uint width = 0;
	uint height = 0;
	SceneColor.GetDimensions(width, height);

	const uint linearThreadIndex = groupThreadId.y * 16u + groupThreadId.x;
	const uint2 pixel = groupId.xy * 16u + groupThreadId.xy;
	const float2 moments =
	    pixel.x < width && pixel.y < height
	        ? Exposure::BuildLogLuminanceMoment(SceneColor.Load(int3(pixel, 0)).rgb)
	        : 0.0f.xx;

	StoreReducedGroup(linearThreadIndex, groupId.xy, moments);
}

[numthreads(16, 16, 1)] void ReduceTextureMain(uint3 groupId : SV_GroupID, uint3 groupThreadId : SV_GroupThreadID)
{
	uint width = 0;
	uint height = 0;
	LuminanceMomentsInput.GetDimensions(width, height);

	const uint linearThreadIndex = groupThreadId.y * 16u + groupThreadId.x;
	const uint2 pixel = groupId.xy * 16u + groupThreadId.xy;
	const float2 moments =
	    pixel.x < width && pixel.y < height
	        ? LuminanceMomentsInput.Load(int3(pixel, 0)).xy
	        : 0.0f.xx;

	StoreReducedGroup(linearThreadIndex, groupId.xy, moments);
}

#include "/Engine/Passes/PostProcessing/Exposure/ExposureHistogram.hlsli"

RWStructuredBuffer<uint> HistogramCounts;

Texture2D SceneColor;
groupshared uint LocalHistogram[ExposureHistogram::BinCount];

[numthreads(16, 16, 1)]
void main(uint3 groupId : SV_GroupID, uint3 groupThreadId : SV_GroupThreadID)
{
	const uint threadIndex = groupThreadId.y * 16u + groupThreadId.x;
	LocalHistogram[threadIndex] = 0u;
	LocalHistogram[threadIndex + 256u] = 0u;
	GroupMemoryBarrierWithGroupSync();

	uint width;
	uint height;
	SceneColor.GetDimensions(width, height);
	const uint2 pixel = groupId.xy * 16u + groupThreadId.xy;
	if (pixel.x < width && pixel.y < height)
	{
		const float3 color = SceneColor.Load(int3(pixel, 0)).rgb;
		if (all(isfinite(color)))
		{
			const float luminance = CommonColor::LuminanceRec709(CommonColor::ClampPositive(color));
			InterlockedAdd(LocalHistogram[ExposureHistogram::FindBin(luminance)], 1u);
		}
	}
	GroupMemoryBarrierWithGroupSync();

	[unroll(2)]
	for (uint bin = threadIndex; bin < ExposureHistogram::BinCount; bin += 256u)
	{
		if (LocalHistogram[bin] != 0u)
		{
			InterlockedAdd(HistogramCounts[bin], LocalHistogram[bin]);
		}
	}
}

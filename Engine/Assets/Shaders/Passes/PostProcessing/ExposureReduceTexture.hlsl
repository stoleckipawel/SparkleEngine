#include "/Engine/Passes/PostProcessing/ExposureReduceCommon.hlsli"

Texture2D LuminanceMomentsInput;

[numthreads(16, 16, 1)]
void main(uint3 groupId : SV_GroupID, uint3 groupThreadId : SV_GroupThreadID)
{
	uint width = 0u;
	uint height = 0u;
	LuminanceMomentsInput.GetDimensions(width, height);

	const uint linearThreadIndex = groupThreadId.y * 16u + groupThreadId.x;
	const uint2 pixel = groupId.xy * 16u + groupThreadId.xy;
	const float2 moments = pixel.x < width && pixel.y < height ? LuminanceMomentsInput.Load(int3(pixel, 0)).xy : 0.0f.xx;
	ExposureReduce::StoreGroup(linearThreadIndex, groupId.xy, moments);
}

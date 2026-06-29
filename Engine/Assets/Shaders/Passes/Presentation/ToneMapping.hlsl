#include "Display/ToneMapping.hlsli"

Texture2D SceneColor;
Texture2D ExposureTexture;
RWTexture2D<float4> ToneMappedColor;
SamplerState SamplerLinearClamp;

cbuffer ToneMappingUniformData
{
	uint ToneMapper;
	uint ToneMappingPadding0;
	uint ToneMappingPadding1;
	uint ToneMappingPadding2;
};

[numthreads(8, 8, 1)] void main(uint3 dispatchThreadId : SV_DispatchThreadID)
{
	uint width;
	uint height;
	ToneMappedColor.GetDimensions(width, height);
	const uint2 pixelCoord = dispatchThreadId.xy;
	if (pixelCoord.x >= width || pixelCoord.y >= height)
	{
		return;
	}

	const float2 uv = (float2(pixelCoord) + float2(0.5f, 0.5f)) / float2(width, height);
	const float4 sceneColor = SceneColor.SampleLevel(SamplerLinearClamp, uv, 0.0f);
	const float exposure = max(ExposureTexture.Load(int3(0, 0, 0)).r, 0.0f);
	const float3 displayLinear = ToneMapping::ApplyToneMapper(sceneColor.rgb * exposure, ToneMapper);
	ToneMappedColor[pixelCoord] = float4(displayLinear, saturate(sceneColor.a));
}

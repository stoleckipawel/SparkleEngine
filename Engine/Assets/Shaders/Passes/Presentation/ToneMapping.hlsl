#include "/Engine/Display/ToneMapping.hlsli"

Texture2D SceneColor;
Texture2D ExposureTexture;
RWTexture2D<float4> ToneMappedColor;

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

	const float4 sceneColor = SceneColor.Load(int3(pixelCoord, 0));
	const float exposure = max(ExposureTexture.Load(int3(0, 0, 0)).r, 0.0f);
	const float3 displayLinear = ToneMapping::ApplyToneMapper(sceneColor.rgb * exposure, ToneMapper);
	ToneMappedColor[pixelCoord] = float4(displayLinear, saturate(sceneColor.a));
}

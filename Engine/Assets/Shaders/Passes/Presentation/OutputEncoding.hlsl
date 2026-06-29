#include "Display/OutputEncoding.hlsli"

Texture2D DisplayLinearColor;
RWTexture2D<float4> EncodedColor;
SamplerState SamplerLinearClamp;

cbuffer OutputEncodingUniformData
{
	uint OutputColorEncoding;
	uint OutputEncodingPadding0;
	uint OutputEncodingPadding1;
	uint OutputEncodingPadding2;
};

[numthreads(8, 8, 1)] void main(uint3 dispatchThreadId : SV_DispatchThreadID)
{
	uint width;
	uint height;
	EncodedColor.GetDimensions(width, height);
	const uint2 pixelCoord = dispatchThreadId.xy;
	if (pixelCoord.x >= width || pixelCoord.y >= height)
	{
		return;
	}

	const float2 uv = (float2(pixelCoord) + float2(0.5f, 0.5f)) / float2(width, height);
	const float4 displayLinear = DisplayLinearColor.SampleLevel(SamplerLinearClamp, uv, 0.0f);
	const float3 encodedColor = OutputEncoding::Encode(displayLinear.rgb, OutputColorEncoding);
	EncodedColor[pixelCoord] = float4(encodedColor, saturate(displayLinear.a));
}

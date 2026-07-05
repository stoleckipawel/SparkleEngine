#include "Display/OutputEncoding.hlsli"

Texture2D DisplayLinearColor;
RWTexture2D<float4> EncodedColor;

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

	const float4 displayLinear = DisplayLinearColor.Load(int3(pixelCoord, 0));
	const float3 encodedColor = OutputEncoding::Encode(displayLinear.rgb, OutputColorEncoding);
	EncodedColor[pixelCoord] = float4(encodedColor, saturate(displayLinear.a));
}

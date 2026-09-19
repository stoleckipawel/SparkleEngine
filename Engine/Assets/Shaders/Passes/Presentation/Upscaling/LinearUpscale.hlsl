Texture2D ScalingInputColor;
SamplerState SamplerLinearClamp;
RWTexture2D<float4> ScalingOutputColor;

[numthreads(8, 8, 1)]
void main(uint3 dispatchThreadId : SV_DispatchThreadID)
{
	uint width;
	uint height;
	ScalingOutputColor.GetDimensions(width, height);
	const uint2 pixelCoord = dispatchThreadId.xy;
	if (pixelCoord.x >= width || pixelCoord.y >= height)
	{
		return;
	}

	const float2 uv = (float2(pixelCoord) + float2(0.5f, 0.5f)) / float2(width, height);
	ScalingOutputColor[pixelCoord] = ScalingInputColor.SampleLevel(SamplerLinearClamp, uv, 0.0f);
}

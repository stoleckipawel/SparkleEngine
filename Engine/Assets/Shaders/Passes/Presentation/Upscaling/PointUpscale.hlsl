Texture2D ScalingInputColor;
RWTexture2D<float4> ScalingOutputColor;

[numthreads(8, 8, 1)]
void main(uint3 dispatchThreadId : SV_DispatchThreadID)
{
	uint inputWidth = 0u;
	uint inputHeight = 0u;
	ScalingInputColor.GetDimensions(inputWidth, inputHeight);

	uint outputWidth = 0u;
	uint outputHeight = 0u;
	ScalingOutputColor.GetDimensions(outputWidth, outputHeight);

	const uint2 outputPixel = dispatchThreadId.xy;
	if (outputPixel.x >= outputWidth || outputPixel.y >= outputHeight)
	{
		return;
	}

	const float2 uv = (float2(outputPixel) + 0.5f) / float2(outputWidth, outputHeight);
	const uint2 inputPixel = min(uint2(uv * float2(inputWidth, inputHeight)), uint2(inputWidth - 1u, inputHeight - 1u));
	ScalingOutputColor[outputPixel] = ScalingInputColor.Load(int3(inputPixel, 0));
}

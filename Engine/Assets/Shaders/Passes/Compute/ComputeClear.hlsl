RWTexture2D<float4> OutputTexture;

[numthreads(8, 8, 1)] void main(uint3 dispatchThreadId : SV_DispatchThreadID)
{
	uint width = 0;
	uint height = 0;
	OutputTexture.GetDimensions(width, height);

	if (dispatchThreadId.x < width && dispatchThreadId.y < height)
	{
		OutputTexture[dispatchThreadId.xy] = 0.0f.xxxx;
	}
}

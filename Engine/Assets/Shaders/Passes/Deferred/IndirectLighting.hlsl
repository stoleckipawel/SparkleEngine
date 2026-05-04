RWTexture2D<float4> IndirectDiffuseTexture;
RWTexture2D<float4> IndirectSpecularTexture;
RWTexture2D<float4> IndirectSubsurfaceTexture;

[numthreads(8, 8, 1)] void main(uint3 dispatchThreadId : SV_DispatchThreadID)
{
	uint width = 0;
	uint height = 0;
	IndirectDiffuseTexture.GetDimensions(width, height);

	if (dispatchThreadId.x >= width || dispatchThreadId.y >= height)
	{
		return;
	}

	IndirectDiffuseTexture[dispatchThreadId.xy] = 0.0f.xxxx;
	IndirectSpecularTexture[dispatchThreadId.xy] = 0.0f.xxxx;
	IndirectSubsurfaceTexture[dispatchThreadId.xy] = 0.0f.xxxx;
}
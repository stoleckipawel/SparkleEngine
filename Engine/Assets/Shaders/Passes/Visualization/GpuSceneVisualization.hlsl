RWTexture2D<float4> SceneColor;
Texture2D GBufferBaseColor;

[numthreads(8, 8, 1)]
void main(uint3 dispatchThreadId : SV_DispatchThreadID)
{
	uint width = 0;
	uint height = 0;
	SceneColor.GetDimensions(width, height);

	if (dispatchThreadId.x >= width || dispatchThreadId.y >= height)
	{
		return;
	}

	const float4 instanceColor = GBufferBaseColor.Load(int3(dispatchThreadId.xy, 0));
	SceneColor[dispatchThreadId.xy] = float4(saturate(instanceColor.rgb), instanceColor.a);
}

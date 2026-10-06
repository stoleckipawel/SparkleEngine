#include "/Engine/Passes/GBuffer/SceneDepthUtils.hlsli"

RWTexture2D<float4> SceneColor;
Texture2D DirectDiffuse;
Texture2D DirectSpecular;
Texture2D DirectSubsurface;
Texture2D IndirectDiffuse;
Texture2D IndirectSpecular;
Texture2D GBufferBaseColor;
Texture2D GBufferDeviceZ;
Texture2D GBufferEmissive;

[numthreads(8, 8, 1)]
void main(uint3 dispatchThreadId : SV_DispatchThreadID)
{
	uint width, height;
	SceneColor.GetDimensions(width, height);
	if (dispatchThreadId.x >= width || dispatchThreadId.y >= height)
	{
		return;
	}
	const int3 pixel = int3(dispatchThreadId.xy, 0);
	if (SceneDepthUtils::IsSkyDeviceZ(GBufferDeviceZ.Load(pixel).r))
	{
		return;
	}
	float3 lit = max(GBufferEmissive.Load(pixel).rgb, 0.0f);
	lit += DirectDiffuse.Load(pixel).rgb + DirectSpecular.Load(pixel).rgb + DirectSubsurface.Load(pixel).rgb;
	lit += IndirectDiffuse.Load(pixel).rgb + IndirectSpecular.Load(pixel).rgb;
	SceneColor[dispatchThreadId.xy] = float4(lit, GBufferBaseColor.Load(pixel).a);
}

#include "/Engine/Geometry/ScreenSpace.hlsli"
#include "/Engine/Lighting/Sky.hlsli"
#include "/Engine/Passes/GBuffer/GBufferUtils.hlsli"

RWTexture2D<float4> SceneColor;
Texture2D SkyTexture;
SamplerState SamplerLinearClamp;

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

	const float sceneDepth = SceneDepth.Load(int3(dispatchThreadId.xy, 0)).r;
	if (!IsSkyPixel(sceneDepth))
	{
		return;
	}

	const float3 worldDirection = ComputeSkyViewDirectionWorld(dispatchThreadId.xy);
	const float3 skyColor = SampleSkyRadiance(SkyTexture, SamplerLinearClamp, worldDirection);
	SceneColor[dispatchThreadId.xy] = float4(skyColor, 1.0f);
}

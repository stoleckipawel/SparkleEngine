#include "Resources/ConstantBuffers.hlsli"
#include "Passes/Deferred/GBufferUtils.hlsli"

RWTexture2D<float4> SceneColorTexture;
Texture2D SkyTexture;
SamplerState SamplerLinearClamp;

float3 ComputeSkyDirection(uint2 pixelCoord)
{
	const float2 uv = (float2(pixelCoord) + 0.5f) * ViewportSizeInv;
	const float2 ndc = float2(uv.x * 2.0f - 1.0f, 1.0f - uv.y * 2.0f);
	const float4 positionClip = float4(ndc, 1.0f, 1.0f);
	const float4 positionView = mul(positionClip, Camera.InvProjectionMTX);
	const float3 viewDirection = normalize(positionView.xyz / max(positionView.w, 1.0e-6f));
	return normalize(mul(float4(viewDirection, 0.0f), Camera.InvViewMTX).xyz);
}

float2 ComputeSkyUv(float3 worldDirection)
{
	const float clampedY = clamp(worldDirection.y, -1.0f, 1.0f);
	const float u = atan2(-worldDirection.z, worldDirection.x) * 0.15915494309189535f + 0.5f;
	const float v = acos(clampedY) * 0.3183098861837907f;
	return float2(frac(u), saturate(v));
}

[numthreads(8, 8, 1)] void main(uint3 dispatchThreadId : SV_DispatchThreadID)
{
	uint width = 0;
	uint height = 0;
	SceneColorTexture.GetDimensions(width, height);

	if (dispatchThreadId.x >= width || dispatchThreadId.y >= height)
	{
		return;
	}

	const float deviceZ = GBufferDeviceZ.Load(int3(dispatchThreadId.xy, 0)).r;
	if (!IsSkyPixel(deviceZ))
	{
		return;
	}

	const float3 worldDirection = ComputeSkyDirection(dispatchThreadId.xy);
	const float2 skyUv = ComputeSkyUv(worldDirection);
	const float3 skyRadiance = SkyTexture.SampleLevel(SamplerLinearClamp, skyUv, 0.0f).rgb;
	const float3 skyColor = skyRadiance / (skyRadiance + 1.0f.xxx);
	SceneColorTexture[dispatchThreadId.xy] = float4(skyColor, 1.0f);
}
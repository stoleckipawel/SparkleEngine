#include "Resources/Samplers.hlsli"

RWTexture2D<float4> IndirectDiffuseTexture;
Texture2D GBufferNormal;
Texture2D GBufferDeviceZ;
Texture2D SkyTexture;

bool IsSkyPixel(float deviceZ)
{
	return deviceZ <= 1.0e-6f;
}

float3 DecodeGBufferNormal(float3 normalWorld)
{
	const float lengthSquared = dot(normalWorld, normalWorld);
	return lengthSquared > 0.0f ? normalWorld * rsqrt(lengthSquared) : float3(0.0f, 0.0f, 1.0f);
}

float2 ComputeSkyUv(float3 worldDirection)
{
	const float clampedY = clamp(worldDirection.y, -1.0f, 1.0f);
	const float u = atan2(-worldDirection.z, worldDirection.x) * 0.15915494309189535f + 0.5f;
	const float v = acos(clampedY) * 0.3183098861837907f;
	return float2(u, v);
}

float3 SampleSkyAmbient(float3 normalWorld)
{
	const float3 skyRadiance = SkyTexture.SampleLevel(SamplerLinearNoMipClamp, ComputeSkyUv(normalize(normalWorld)), 0.0f).rgb;
	const float3 skyColor = skyRadiance / (skyRadiance + 1.0f.xxx);
	const float horizonWeight = saturate(normalWorld.y * 0.5f + 0.5f);
	const float3 groundBounce = float3(0.035f, 0.032f, 0.028f);
	return lerp(groundBounce, skyColor, horizonWeight) * 1.35f;
}

[numthreads(8, 8, 1)] void main(uint3 dispatchThreadId : SV_DispatchThreadID)
{
	uint width = 0;
	uint height = 0;
	IndirectDiffuseTexture.GetDimensions(width, height);

	if (dispatchThreadId.x >= width || dispatchThreadId.y >= height)
	{
		return;
	}

	const float deviceZ = GBufferDeviceZ.Load(int3(dispatchThreadId.xy, 0)).r;
	if (IsSkyPixel(deviceZ))
	{
		IndirectDiffuseTexture[dispatchThreadId.xy] = 0.0f.xxxx;
		return;
	}

	const float3 normalWorld = DecodeGBufferNormal(GBufferNormal.Load(int3(dispatchThreadId.xy, 0)).xyz);
	const float3 ambientDiffuse = SampleSkyAmbient(normalWorld);

	IndirectDiffuseTexture[dispatchThreadId.xy] = float4(ambientDiffuse, 1.0f);
}

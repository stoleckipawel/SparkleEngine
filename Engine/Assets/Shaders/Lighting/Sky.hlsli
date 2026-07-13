#ifndef SPARKLE_LIGHTING_SKY_HLSLI
#define SPARKLE_LIGHTING_SKY_HLSLI

#include "Lighting/SkyUniform.hlsli"

float2 ComputeSkyUv(float3 worldDirection)
{
	const float3 direction = normalize(worldDirection);
	const float clampedY = clamp(direction.y, -1.0f, 1.0f);
	const float u = atan2(-direction.z, direction.x) * 0.15915494309189535f + 0.5f;
	const float v = acos(clampedY) * 0.3183098861837907f;
	return float2(frac(u), saturate(v));
}

float3 SampleSkyRadiance(Texture2D skyTexture, SamplerState skySampler, float3 worldDirection)
{
	const float3 radiance = skyTexture.SampleLevel(skySampler, ComputeSkyUv(worldDirection), 0.0f).rgb;
	return SkyEnabled != 0u ? radiance * SkyColor * SkyIntensity : 0.0f.xxx;
}

#endif

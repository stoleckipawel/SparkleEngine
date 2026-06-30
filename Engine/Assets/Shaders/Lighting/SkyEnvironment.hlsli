#ifndef SPARKLE_LIGHTING_SKY_ENVIRONMENT_HLSLI
#define SPARKLE_LIGHTING_SKY_ENVIRONMENT_HLSLI

float2 ComputeSkyEnvironmentUv(float3 worldDirection)
{
	const float3 direction = normalize(worldDirection);
	const float clampedY = clamp(direction.y, -1.0f, 1.0f);
	const float u = atan2(-direction.z, direction.x) * 0.15915494309189535f + 0.5f;
	const float v = acos(clampedY) * 0.3183098861837907f;
	return float2(frac(u), saturate(v));
}

float3 SampleSkyEnvironment(Texture2D environmentTexture, SamplerState environmentSampler, float3 worldDirection)
{
	return environmentTexture.SampleLevel(environmentSampler, ComputeSkyEnvironmentUv(worldDirection), 0.0f).rgb;
}

#endif

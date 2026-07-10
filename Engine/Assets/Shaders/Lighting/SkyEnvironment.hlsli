#ifndef SPARKLE_LIGHTING_SKY_ENVIRONMENT_HLSLI
#define SPARKLE_LIGHTING_SKY_ENVIRONMENT_HLSLI

#include "Geometry/ScreenSpace.hlsli"

float2 ComputeSkyEnvironmentUv(float3 worldDirection)
{
	const float3 direction = normalize(worldDirection);
	const float clampedY = clamp(direction.y, -1.0f, 1.0f);
	const float u = atan2(-direction.z, direction.x) * 0.15915494309189535f + 0.5f;
	const float v = acos(clampedY) * 0.3183098861837907f;
	return float2(frac(u), saturate(v));
}

float3 SampleSkyEnvironmentRadiance(Texture2D environmentTexture, SamplerState environmentSampler, float3 worldDirection)
{
	return max(environmentTexture.SampleLevel(environmentSampler, ComputeSkyEnvironmentUv(worldDirection), 0.0f).rgb, 0.0f.xxx);
}

float3 ComputeSkyViewDirectionWorld(uint2 pixelCoord)
{
	const float2 ndc = PixelCenterToUnjitteredNdc(pixelCoord);
	const float4 positionClip = float4(ndc, 1.0f, 1.0f);
	const float4 positionView = mul(positionClip, Camera.InvProjectionMTX);
	const float3 viewDirection = normalize(positionView.xyz / max(positionView.w, 1.0e-6f));
	return normalize(mul(float4(viewDirection, 0.0f), Camera.InvViewMTX).xyz);
}

#endif

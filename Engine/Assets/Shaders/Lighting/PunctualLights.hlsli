#ifndef SPARKLE_PUNCTUAL_LIGHTS_HLSLI
#define SPARKLE_PUNCTUAL_LIGHTS_HLSLI

#include "Resources/ConstantBuffers.hlsli"

namespace PunctualLights
{
	float3 GetDirectionalLightDirection(uint lightIndex)
	{
		return normalize(-DirectionalLights[lightIndex].Direction);
	}

	float3 GetPointLightDirection(float3 positionWorld, uint lightIndex, out float distanceToLight)
	{
		const float3 surfaceToLight = PointLights[lightIndex].Position - positionWorld;
		distanceToLight = length(surfaceToLight);
		return surfaceToLight / max(distanceToLight, 0.0001f);
	}

	float3 GetSpotLightDirection(float3 positionWorld, uint lightIndex, out float distanceToLight)
	{
		const float3 surfaceToLight = SpotLights[lightIndex].Position - positionWorld;
		distanceToLight = length(surfaceToLight);
		return surfaceToLight / max(distanceToLight, 0.0001f);
	}

	float ComputeDistanceAttenuation(float distanceToLight, float range)
	{
		const float inverseSquare = rcp(max(distanceToLight * distanceToLight, 0.01f));
		if (range <= 0.0f)
		{
			return inverseSquare;
		}

		const float rangeFactor = saturate(1.0f - distanceToLight / range);
		return inverseSquare * rangeFactor * rangeFactor;
	}

	float ComputeSpotConeAttenuation(float3 lightToSurfaceDirection, float3 spotDirection, float innerConeCosine, float outerConeCosine)
	{
		const float coneCosine = dot(normalize(lightToSurfaceDirection), normalize(spotDirection));
		const float coneRange = max(innerConeCosine - outerConeCosine, 0.0001f);
		return saturate((coneCosine - outerConeCosine) / coneRange);
	}
}

#endif

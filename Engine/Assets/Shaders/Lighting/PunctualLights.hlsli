#ifndef SPARKLE_PUNCTUAL_LIGHTS_HLSLI
#define SPARKLE_PUNCTUAL_LIGHTS_HLSLI

#include "Resources/LightGpuData.hlsli"
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

	float ComputeRangeAttenuation(float distanceToLight, float range)
	{
		if (range <= 0.0f)
		{
			return 1.0f;
		}

		const float normalizedDistance = distanceToLight / max(range, 1.0e-4f);
		const float smoothRange = saturate(1.0f - normalizedDistance * normalizedDistance * normalizedDistance * normalizedDistance);
		return smoothRange * smoothRange;
	}

	float ComputeDistanceAttenuationDenominator(float distanceToLight, float3 distanceAttenuationCoefficients)
	{
		return distanceAttenuationCoefficients.x + distanceAttenuationCoefficients.y * distanceToLight
		    + distanceAttenuationCoefficients.z * distanceToLight * distanceToLight;
	}

	float ComputePunctualDistanceAttenuation(float distanceToLight, float range, float3 distanceAttenuationCoefficients)
	{
		const float denominator = ComputeDistanceAttenuationDenominator(distanceToLight, distanceAttenuationCoefficients);
		return rcp(max(denominator, 1.0e-4f)) * ComputeRangeAttenuation(distanceToLight, range);
	}

	float ComputeAreaDistanceAttenuationCorrection(float distanceToLight, float3 distanceAttenuationCoefficients)
	{
		const float distanceSquared = max(distanceToLight * distanceToLight, 1.0e-4f);
		const float denominator = ComputeDistanceAttenuationDenominator(distanceToLight, distanceAttenuationCoefficients);
		return distanceSquared / max(denominator, 1.0e-4f);
	}

	float ComputeSpotAngularAttenuation(float3 lightToSurfaceDirection,
	                                    float3 spotDirection,
	                                    float innerAngleCosine,
	                                    float outerAngleCosine)
	{
		const float coneCosine = dot(normalize(lightToSurfaceDirection), normalize(spotDirection));
		const float angularTransition = max(innerAngleCosine - outerAngleCosine, 0.0001f);
		const float angularAttenuation = saturate((coneCosine - outerAngleCosine) / angularTransition);
		return angularAttenuation * angularAttenuation;
	}
}

#endif

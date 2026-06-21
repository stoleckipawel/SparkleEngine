#ifndef SPARKLE_DIRECT_LIGHTING_COMMON_HLSLI
#define SPARKLE_DIRECT_LIGHTING_COMMON_HLSLI

#include "Resources/ConstantBuffers.hlsli"
#include "BRDF/BRDF.hlsli"

namespace DirectLighting
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

	void EvaluateDirectLight(
	    float3 viewDirWorld,
	    float3 normalWorld,
	    float roughness,
	    bool evaluateSubsurface,
	    float3 lightDirection,
	    float3 radiance,
	    out float3 outDiffuse,
	    out float3 outSpecular,
	    out float3 outSubsurface)
	{
		BRDF::ShadingData shadingData = BRDF::ComputeShadingData(normalWorld, viewDirWorld, lightDirection);

		if (shadingData.NoL <= 0.0f || shadingData.NoV <= 0.0f)
		{
			outDiffuse = 0.0f;
			outSpecular = 0.0f;
			outSubsurface = 0.0f;
			return;
		}

		const float clampedRoughness = max(roughness, 0.04f);

		outDiffuse = BRDF::Diffuse::EvaluateDirectTransport(clampedRoughness, shadingData);
		outSpecular = BRDF::Specular::EvaluateDirectTransport(shadingData, clampedRoughness);
		outSubsurface = 0.0f.xxx;

		outDiffuse *= radiance * shadingData.NoL;
		outSpecular *= radiance * shadingData.NoL;
		if (evaluateSubsurface)
		{
			outSubsurface = BRDF::Subsurface::EvaluateDirectTransport(clampedRoughness, shadingData) * radiance * shadingData.NoL;
		}
	}

	void AccumulateDirectionalLight(
	    float3 viewDirWorld,
	    float3 normalWorld,
	    float roughness,
	    bool evaluateSubsurface,
	    uint lightIndex,
	    float shadowVisibility,
	    out float3 outDiffuse,
	    out float3 outSpecular,
	    out float3 outSubsurface)
	{
		const float3 lightDirection = GetDirectionalLightDirection(lightIndex);
		const float3 radiance =
		    DirectionalLights[lightIndex].Color * DirectionalLights[lightIndex].Intensity * shadowVisibility;
		EvaluateDirectLight(viewDirWorld, normalWorld, roughness, evaluateSubsurface, lightDirection, radiance, outDiffuse, outSpecular, outSubsurface);
	}

	void AccumulatePointLight(
	    float3 positionWorld,
	    float3 viewDirWorld,
	    float3 normalWorld,
	    float roughness,
	    bool evaluateSubsurface,
	    uint lightIndex,
	    float shadowVisibility,
	    out float3 outDiffuse,
	    out float3 outSpecular,
	    out float3 outSubsurface)
	{
		const PointLightConstantBufferData light = PointLights[lightIndex];
		float distanceToLight = 0.0f;
		const float3 lightDirection = GetPointLightDirection(positionWorld, lightIndex, distanceToLight);
		const float attenuation = ComputeDistanceAttenuation(distanceToLight, light.Range);
		const float3 radiance = light.Color * light.Intensity * attenuation * shadowVisibility;
		EvaluateDirectLight(viewDirWorld, normalWorld, roughness, evaluateSubsurface, lightDirection, radiance, outDiffuse, outSpecular, outSubsurface);
	}

	void AccumulateSpotLight(
	    float3 positionWorld,
	    float3 viewDirWorld,
	    float3 normalWorld,
	    float roughness,
	    bool evaluateSubsurface,
	    uint lightIndex,
	    float shadowVisibility,
	    out float3 outDiffuse,
	    out float3 outSpecular,
	    out float3 outSubsurface)
	{
		const SpotLightConstantBufferData light = SpotLights[lightIndex];
		float distanceToLight = 0.0f;
		const float3 lightDirection = GetSpotLightDirection(positionWorld, lightIndex, distanceToLight);
		const float3 lightToSurfaceDirection = -lightDirection;
		const float distanceAttenuation = ComputeDistanceAttenuation(distanceToLight, light.Range);
		const float coneAttenuation = ComputeSpotConeAttenuation(lightToSurfaceDirection, light.Direction, light.InnerConeCosine, light.OuterConeCosine);
		const float3 radiance = light.Color * light.Intensity * distanceAttenuation * coneAttenuation * shadowVisibility;
		EvaluateDirectLight(viewDirWorld, normalWorld, roughness, evaluateSubsurface, lightDirection, radiance, outDiffuse, outSpecular, outSubsurface);
	}
}

#endif

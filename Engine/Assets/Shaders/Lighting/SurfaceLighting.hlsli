#ifndef SPARKLE_SURFACE_LIGHTING_HLSLI
#define SPARKLE_SURFACE_LIGHTING_HLSLI

#include "BRDF/BRDF.hlsli"
#include "Lighting/PunctualLights.hlsli"

namespace SurfaceLighting
{
	float3 BuildF0(float3 baseColor, float metallic, float dielectricF0)
	{
		return lerp(saturate(dielectricF0).xxx, saturate(baseColor), saturate(metallic));
	}

	void EvaluateDirectLightWithF0(
	    float3 viewDirWorld,
	    float3 normalWorld,
	    float3 baseColor,
	    float roughness,
	    float metallic,
	    float3 f0,
	    float3 subsurfaceColor,
	    float subsurfaceStrength,
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

		BRDF::Direct::Evaluate(
		    shadingData,
		    baseColor,
		    roughness,
		    metallic,
		    f0,
		    subsurfaceColor,
		    evaluateSubsurface ? subsurfaceStrength : 0.0f,
		    outDiffuse,
		    outSpecular,
		    outSubsurface);

		outDiffuse *= radiance * shadingData.NoL;
		outSpecular *= radiance * shadingData.NoL;
		outSubsurface *= radiance * shadingData.NoL;
	}

	void EvaluateDirectLight(
	    float3 viewDirWorld,
	    float3 normalWorld,
	    float3 baseColor,
	    float roughness,
	    float metallic,
	    float dielectricF0,
	    float3 subsurfaceColor,
	    float subsurfaceStrength,
	    bool evaluateSubsurface,
	    float3 lightDirection,
	    float3 radiance,
	    out float3 outDiffuse,
	    out float3 outSpecular,
	    out float3 outSubsurface)
	{
		const float3 f0 = BuildF0(baseColor, metallic, dielectricF0);
		EvaluateDirectLightWithF0(
		    viewDirWorld,
		    normalWorld,
		    baseColor,
		    roughness,
		    metallic,
		    f0,
		    subsurfaceColor,
		    subsurfaceStrength,
		    evaluateSubsurface,
		    lightDirection,
		    radiance,
		    outDiffuse,
		    outSpecular,
		    outSubsurface);
	}

	void AccumulateDirectionalLight(
	    float3 viewDirWorld,
	    float3 normalWorld,
	    float3 baseColor,
	    float roughness,
	    float metallic,
	    float dielectricF0,
	    float3 subsurfaceColor,
	    float subsurfaceStrength,
	    bool evaluateSubsurface,
	    uint lightIndex,
	    float shadowVisibility,
	    out float3 outDiffuse,
	    out float3 outSpecular,
	    out float3 outSubsurface)
	{
		const float3 lightDirection = PunctualLights::GetDirectionalLightDirection(lightIndex);
		const float3 radiance =
		    DirectionalLights[lightIndex].Color * DirectionalLights[lightIndex].Intensity * shadowVisibility;
		EvaluateDirectLight(
		    viewDirWorld,
		    normalWorld,
		    baseColor,
		    roughness,
		    metallic,
		    dielectricF0,
		    subsurfaceColor,
		    subsurfaceStrength,
		    evaluateSubsurface,
		    lightDirection,
		    radiance,
		    outDiffuse,
		    outSpecular,
		    outSubsurface);
	}

	void AccumulatePointLight(
	    float3 positionWorld,
	    float3 viewDirWorld,
	    float3 normalWorld,
	    float3 baseColor,
	    float roughness,
	    float metallic,
	    float dielectricF0,
	    float3 subsurfaceColor,
	    float subsurfaceStrength,
	    bool evaluateSubsurface,
	    uint lightIndex,
	    float shadowVisibility,
	    out float3 outDiffuse,
	    out float3 outSpecular,
	    out float3 outSubsurface)
	{
		const PointLightConstantBufferData light = PointLights[lightIndex];
		float distanceToLight = 0.0f;
		const float3 lightDirection = PunctualLights::GetPointLightDirection(positionWorld, lightIndex, distanceToLight);
		const float attenuation = PunctualLights::ComputeDistanceAttenuation(distanceToLight, light.Range);
		const float3 radiance = light.Color * light.Intensity * attenuation * shadowVisibility;
		EvaluateDirectLight(
		    viewDirWorld,
		    normalWorld,
		    baseColor,
		    roughness,
		    metallic,
		    dielectricF0,
		    subsurfaceColor,
		    subsurfaceStrength,
		    evaluateSubsurface,
		    lightDirection,
		    radiance,
		    outDiffuse,
		    outSpecular,
		    outSubsurface);
	}

	void AccumulateSpotLight(
	    float3 positionWorld,
	    float3 viewDirWorld,
	    float3 normalWorld,
	    float3 baseColor,
	    float roughness,
	    float metallic,
	    float dielectricF0,
	    float3 subsurfaceColor,
	    float subsurfaceStrength,
	    bool evaluateSubsurface,
	    uint lightIndex,
	    float shadowVisibility,
	    out float3 outDiffuse,
	    out float3 outSpecular,
	    out float3 outSubsurface)
	{
		const SpotLightConstantBufferData light = SpotLights[lightIndex];
		float distanceToLight = 0.0f;
		const float3 lightDirection = PunctualLights::GetSpotLightDirection(positionWorld, lightIndex, distanceToLight);
		const float3 lightToSurfaceDirection = -lightDirection;
		const float distanceAttenuation = PunctualLights::ComputeDistanceAttenuation(distanceToLight, light.Range);
		const float coneAttenuation =
		    PunctualLights::ComputeSpotConeAttenuation(lightToSurfaceDirection, light.Direction, light.InnerConeCosine, light.OuterConeCosine);
		const float3 radiance = light.Color * light.Intensity * distanceAttenuation * coneAttenuation * shadowVisibility;
		EvaluateDirectLight(
		    viewDirWorld,
		    normalWorld,
		    baseColor,
		    roughness,
		    metallic,
		    dielectricF0,
		    subsurfaceColor,
		    subsurfaceStrength,
		    evaluateSubsurface,
		    lightDirection,
		    radiance,
		    outDiffuse,
		    outSpecular,
		    outSubsurface);
	}
}

#endif

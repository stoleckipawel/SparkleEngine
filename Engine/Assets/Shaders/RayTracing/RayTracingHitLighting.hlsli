#pragma once

#include "Passes/Deferred/DirectLightingCommon.hlsli"
#include "RayTracing/RayTracingMaterialHit.hlsli"

void AccumulateRayTracingHitDirectLight(
    RayTracingHitSurfaceData surface,
    float3 viewDirWorld,
    float3 lightDirection,
    float3 radiance,
    inout float3 incidentRadiance)
{
	BRDF::ShadingData shadingData = BRDF::ComputeShadingData(surface.NormalWorld, viewDirWorld, lightDirection);
	if (shadingData.NoL <= 0.0f || shadingData.NoV <= 0.0f)
	{
		return;
	}

	float3 diffuse = 0.0f.xxx;
	float3 specular = 0.0f.xxx;
	float3 subsurface = 0.0f.xxx;
	const float safeRoughness = max(surface.Roughness, 1.0e-4f);
	const float3 f0 = lerp(surface.DielectricF0.xxx, surface.BaseColor, surface.Metallic);
	BRDF::Direct::Evaluate(
	    shadingData,
	    surface.BaseColor,
	    safeRoughness,
	    surface.Metallic,
	    f0,
	    surface.SubsurfaceColor,
	    surface.SubsurfaceStrength,
	    diffuse,
	    specular,
	    subsurface);

	incidentRadiance += (diffuse + specular + subsurface) * radiance * shadingData.NoL;
}

float3 ShadeRayTracingHitIncidentRadiance(RayTracingHitSurfaceData surface, float3 rayDirectionWorld)
{
	if (!surface.Valid)
	{
		return 0.0f.xxx;
	}

	float3 incidentRadiance = max(surface.EmissiveColor, 0.0f.xxx);
	const float3 viewDirWorld = normalize(-rayDirectionWorld);
	const uint directionalLightCount = ViewLighting.DirectionalLightCount;
	const uint pointLightCount = ViewLighting.PointLightCount;
	const uint spotLightCount = ViewLighting.SpotLightCount;

	[loop] for (uint lightIndex = 0u; lightIndex < directionalLightCount; ++lightIndex)
	{
		const float3 lightDirection = DirectLighting::GetDirectionalLightDirection(lightIndex);
		const float3 radiance = DirectionalLights[lightIndex].Color * DirectionalLights[lightIndex].Intensity;
		AccumulateRayTracingHitDirectLight(surface, viewDirWorld, lightDirection, radiance, incidentRadiance);
	}

	[loop] for (uint lightIndex = 0u; lightIndex < pointLightCount; ++lightIndex)
	{
		const PointLightConstantBufferData light = PointLights[lightIndex];
		float distanceToLight = 0.0f;
		const float3 lightDirection = DirectLighting::GetPointLightDirection(surface.PositionWorld, lightIndex, distanceToLight);
		const float attenuation = DirectLighting::ComputeDistanceAttenuation(distanceToLight, light.Range);
		const float3 radiance = light.Color * light.Intensity * attenuation;
		AccumulateRayTracingHitDirectLight(surface, viewDirWorld, lightDirection, radiance, incidentRadiance);
	}

	[loop] for (uint lightIndex = 0u; lightIndex < spotLightCount; ++lightIndex)
	{
		const SpotLightConstantBufferData light = SpotLights[lightIndex];
		float distanceToLight = 0.0f;
		const float3 lightDirection = DirectLighting::GetSpotLightDirection(surface.PositionWorld, lightIndex, distanceToLight);
		const float3 lightToSurfaceDirection = -lightDirection;
		const float distanceAttenuation = DirectLighting::ComputeDistanceAttenuation(distanceToLight, light.Range);
		const float coneAttenuation =
		    DirectLighting::ComputeSpotConeAttenuation(lightToSurfaceDirection, light.Direction, light.InnerConeCosine, light.OuterConeCosine);
		const float3 radiance = light.Color * light.Intensity * distanceAttenuation * coneAttenuation;
		AccumulateRayTracingHitDirectLight(surface, viewDirWorld, lightDirection, radiance, incidentRadiance);
	}

	return max(incidentRadiance, 0.0f.xxx);
}

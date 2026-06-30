#pragma once

#include "Lighting/PunctualLights.hlsli"
#include "Lighting/SurfaceLighting.hlsli"
#include "RayTracing/RayTracingMaterialHit.hlsli"

void AccumulateRayTracingHitDirectLight(
    RayTracingHitSurfaceData surface,
    float3 viewDirWorld,
    float3 lightDirection,
    float3 radiance,
    inout float3 incidentRadiance)
{
	float3 diffuse = 0.0f.xxx;
	float3 specular = 0.0f.xxx;
	float3 subsurface = 0.0f.xxx;
	const float3 f0 = SurfaceLighting::BuildF0(surface.BaseColor, surface.Metallic, surface.DielectricF0);
	SurfaceLighting::EvaluateDirectLightWithF0(
	    viewDirWorld,
	    surface.NormalWorld,
	    surface.BaseColor,
	    surface.Roughness,
	    surface.Metallic,
	    f0,
	    surface.SubsurfaceColor,
	    surface.SubsurfaceStrength,
	    true,
	    lightDirection,
	    radiance,
	    diffuse,
	    specular,
	    subsurface);

	incidentRadiance += diffuse + specular + subsurface;
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
		const float3 lightDirection = PunctualLights::GetDirectionalLightDirection(lightIndex);
		const float3 radiance = DirectionalLights[lightIndex].Color * DirectionalLights[lightIndex].Intensity;
		AccumulateRayTracingHitDirectLight(surface, viewDirWorld, lightDirection, radiance, incidentRadiance);
	}

	[loop] for (uint lightIndex = 0u; lightIndex < pointLightCount; ++lightIndex)
	{
		const PointLightConstantBufferData light = PointLights[lightIndex];
		float distanceToLight = 0.0f;
		const float3 lightDirection = PunctualLights::GetPointLightDirection(surface.PositionWorld, lightIndex, distanceToLight);
		const float attenuation = PunctualLights::ComputeDistanceAttenuation(distanceToLight, light.Range);
		const float3 radiance = light.Color * light.Intensity * attenuation;
		AccumulateRayTracingHitDirectLight(surface, viewDirWorld, lightDirection, radiance, incidentRadiance);
	}

	[loop] for (uint lightIndex = 0u; lightIndex < spotLightCount; ++lightIndex)
	{
		const SpotLightConstantBufferData light = SpotLights[lightIndex];
		float distanceToLight = 0.0f;
		const float3 lightDirection = PunctualLights::GetSpotLightDirection(surface.PositionWorld, lightIndex, distanceToLight);
		const float3 lightToSurfaceDirection = -lightDirection;
		const float distanceAttenuation = PunctualLights::ComputeDistanceAttenuation(distanceToLight, light.Range);
		const float coneAttenuation =
		    PunctualLights::ComputeSpotConeAttenuation(lightToSurfaceDirection, light.Direction, light.InnerConeCosine, light.OuterConeCosine);
		const float3 radiance = light.Color * light.Intensity * distanceAttenuation * coneAttenuation;
		AccumulateRayTracingHitDirectLight(surface, viewDirWorld, lightDirection, radiance, incidentRadiance);
	}

	return max(incidentRadiance, 0.0f.xxx);
}

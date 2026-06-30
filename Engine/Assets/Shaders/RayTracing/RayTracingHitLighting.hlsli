#pragma once

#include "Lighting/AreaLights.hlsli"
#include "Lighting/SurfaceLighting.hlsli"
#include "RayTracing/RayTracingMaterialHit.hlsli"

void AccumulateRayTracingHitDirectLightSample(
    RayTracingHitSurfaceData surface,
    float3 viewDirWorld,
    LightSampling::DirectLightSample lightSample,
    inout float3 incidentRadiance)
{
	float3 diffuse = 0.0f.xxx;
	float3 specular = 0.0f.xxx;
	float3 subsurface = 0.0f.xxx;
	SurfaceLighting::AccumulateDirectLightSample(
	    viewDirWorld,
	    surface.NormalWorld,
	    surface.BaseColor,
	    surface.Roughness,
	    surface.Metallic,
	    surface.DielectricF0,
	    surface.SubsurfaceColor,
	    surface.SubsurfaceStrength,
	    true,
	    lightSample,
	    1.0f,
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
	const uint rectLightCount = ViewLighting.RectLightCount;

	[loop] for (uint lightIndex = 0u; lightIndex < directionalLightCount; ++lightIndex)
	{
		AccumulateRayTracingHitDirectLightSample(
		    surface,
		    viewDirWorld,
		    AreaLights::SampleDirectionalLight(
		        lightIndex,
		        LightSampling::StableLightSample2D(surface.PositionWorld, lightIndex, 10u, FrameIndex)),
		    incidentRadiance);
	}

	[loop] for (uint lightIndex = 0u; lightIndex < pointLightCount; ++lightIndex)
	{
		AccumulateRayTracingHitDirectLightSample(
		    surface,
		    viewDirWorld,
		    AreaLights::SamplePointLight(
		        surface.PositionWorld,
		        lightIndex,
		        LightSampling::StableLightSample2D(surface.PositionWorld, lightIndex, 11u, FrameIndex)),
		    incidentRadiance);
	}

	[loop] for (uint lightIndex = 0u; lightIndex < spotLightCount; ++lightIndex)
	{
		AccumulateRayTracingHitDirectLightSample(
		    surface,
		    viewDirWorld,
		    AreaLights::SampleSpotLight(
		        surface.PositionWorld,
		        lightIndex,
		        LightSampling::StableLightSample2D(surface.PositionWorld, lightIndex, 12u, FrameIndex)),
		    incidentRadiance);
	}

	[loop] for (uint lightIndex = 0u; lightIndex < rectLightCount; ++lightIndex)
	{
		AccumulateRayTracingHitDirectLightSample(
		    surface,
		    viewDirWorld,
		    AreaLights::SampleRectLight(
		        surface.PositionWorld,
		        lightIndex,
		        LightSampling::StableLightSample2D(surface.PositionWorld, lightIndex, 13u, FrameIndex)),
		    incidentRadiance);
	}

	return max(incidentRadiance, 0.0f.xxx);
}

#pragma once

#include "Lighting/AreaLights.hlsli"
#include "Lighting/SurfaceLighting.hlsli"
#include "RayTracing/RayTracingMaterialHit.hlsli"
#include "RayTracing/Shadows/RayTracedShadowVisibility.hlsli"

void AccumulateRayTracingHitDirectLightSample(
    RayTracingHitSurfaceData surface,
    float3 viewDirWorld,
    LightSampling::DirectLightSample lightSample,
    bool castsShadow,
    inout float3 incidentRadiance)
{
	if (!lightSample.Valid)
	{
		return;
	}

	const ShadowVisibilitySignal shadow =
	    RayTracedShadowVisibility::TraceDirectLightSample(surface.PositionWorld, surface.NormalWorld, lightSample, castsShadow);
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
	    shadow.Visibility,
	    diffuse,
	    specular,
	    subsurface);

	incidentRadiance += diffuse + specular + subsurface;
}

float3 ShadeRayTracingHitIncidentRadiance(
    RayTracingHitSurfaceData surface,
    float3 rayDirectionWorld,
    uint pathSampleIndex,
    uint bounceIndex,
    uint randomFrameIndex)
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
		        LightSampling::StableLightSample2D(
		            surface.PositionWorld,
		            lightIndex,
		            10u + bounceIndex * 4u,
		            randomFrameIndex + pathSampleIndex * 4099u)),
		    DirectionalLights[lightIndex].CastShadow != 0u,
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
		        LightSampling::StableLightSample2D(
		            surface.PositionWorld,
		            lightIndex,
		            11u + bounceIndex * 4u,
		            randomFrameIndex + pathSampleIndex * 4099u)),
		    PointLights[lightIndex].CastShadow != 0u,
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
		        LightSampling::StableLightSample2D(
		            surface.PositionWorld,
		            lightIndex,
		            12u + bounceIndex * 4u,
		            randomFrameIndex + pathSampleIndex * 4099u)),
		    SpotLights[lightIndex].CastShadow != 0u,
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
		        LightSampling::StableLightSample2D(
		            surface.PositionWorld,
		            lightIndex,
		            13u + bounceIndex * 4u,
		            randomFrameIndex + pathSampleIndex * 4099u)),
		    RectLights[lightIndex].CastShadow != 0u,
		    incidentRadiance);
	}

	return max(incidentRadiance, 0.0f.xxx);
}

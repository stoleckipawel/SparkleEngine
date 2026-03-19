#pragma once

#include "Resources/ConstantBuffers.hlsli"
#include "Material/Material.hlsli"
#include "BRDF/BRDF.hlsli"

// =============================================================================
// Lighting System
// =============================================================================
// High-level lighting calculations using BRDF::Direct and BRDF::Indirect.
// Bridges the gap between material properties and the BRDF evaluation API.
// =============================================================================

namespace Lighting
{
	// =========================================================================
	// Light Structures
	// =========================================================================

	struct DirectionalLight
	{
		float3 Direction;  // Normalized, pointing TO the light source
		float3 Radiance;   // Color * intensity (can be HDR)
	};

	struct PointLight
	{
		float3 Position;
		float3 Radiance;
		float Radius;
		bool Enabled;
	};

	// =========================================================================
	// Directional Lights (from PerView constant buffer)
	// =========================================================================

	DirectionalLight GetDirectionalLight(uint lightIndex)
	{
		DirectionalLight light;
		light.Direction =
		    normalize(-ViewLighting.DirectionalLights[lightIndex].Direction);  // CB stores direction toward surface; negate for "toward light"
		light.Radiance = ViewLighting.DirectionalLights[lightIndex].Color * ViewLighting.DirectionalLights[lightIndex].Intensity;
		return light;
	}

	PointLight GetPointLight(uint lightIndex)
	{
		PointLight light;
		light.Position = ViewLighting.PointLights[lightIndex].Position;
		light.Radiance = ViewLighting.PointLights[lightIndex].Color * ViewLighting.PointLights[lightIndex].Intensity;
		light.Radius = ViewLighting.PointLights[lightIndex].Radius;
		light.Enabled = ViewLighting.PointLights[lightIndex].Enabled != 0;
		return light;
	}

	float CalculatePointLightAttenuation(float distanceToLight, float radius)
	{
		if (distanceToLight >= radius)
		{
			return 0.0f;
		}

		const float normalizedDistance = saturate(distanceToLight / max(radius, 0.001f));
		const float smoothFalloff = 1.0f - normalizedDistance * normalizedDistance;
		return (smoothFalloff * smoothFalloff) / max(distanceToLight * distanceToLight, 0.01f);
	}

	void AccumulateDirectLight(
	    float3 positionWorld,
	    float3 viewDirWorld,
	    Material::Properties matProps,
	    float3 lightDirection,
	    float3 lightRadiance,
	    out float3 outDiffuse,
	    out float3 outSpecular,
	    out float3 outSubsurface)
	{
		BRDF::ShadingData sd = BRDF::ComputeShadingData(matProps.NormalWorld, viewDirWorld, lightDirection);

		if (sd.NoL <= 0.0f || sd.NoV <= 0.0f)
		{
			outDiffuse = 0.0f;
			outSpecular = 0.0f;
			outSubsurface = 0.0f;
			return;
		}

		const float roughness = max(matProps.Roughness, 0.04f);
		const float metallic = saturate(matProps.Metallic);
		const float3 F0 = lerp(matProps.DielectricF0.xxx, matProps.BaseColor, metallic);

		float3 diffuseBRDF, specularBRDF, subsurfaceBRDF;
		BRDF::Direct::Evaluate(
		    sd,
		    matProps.BaseColor,
		    roughness,
		    metallic,
		    F0,
		    matProps.SubsurfaceColor,
		    matProps.SubsurfaceStrength,
		    diffuseBRDF,
		    specularBRDF,
		    subsurfaceBRDF);

		outDiffuse = diffuseBRDF * lightRadiance * sd.NoL;
		outSpecular = specularBRDF * lightRadiance * sd.NoL;
		outSubsurface = subsurfaceBRDF * lightRadiance;
	}

	// =========================================================================
	// Direct Lighting
	// =========================================================================
	// Evaluates contribution from analytical light sources (directional, point, spot).

	void CalculateDirect(
	    float3 positionWorld,
	    float3 viewDirWorld,
	    Material::Properties matProps,
	    out float3 outDiffuse,
	    out float3 outSpecular,
	    out float3 outSubsurface)
	{
		outDiffuse = 0.0f;
		outSpecular = 0.0f;
		outSubsurface = 0.0f;

		float3 diffuseContribution, specularContribution, subsurfaceContribution;
		const uint directionalLightCount = min(ViewLighting.DirectionalLightCount, MAX_DIRECTIONAL_LIGHTS);
		const uint pointLightCount = min(ViewLighting.PointLightCount, MAX_POINT_LIGHTS);

		
		[loop]
		for (uint lightIndex = 0; lightIndex < directionalLightCount; ++lightIndex)
		{
			DirectionalLight directionalLight = GetDirectionalLight(lightIndex);
			AccumulateDirectLight(
			    positionWorld,
			    viewDirWorld,
			    matProps,
			    directionalLight.Direction,
			    directionalLight.Radiance,
			    diffuseContribution,
			    specularContribution,
			    subsurfaceContribution);
			outDiffuse += diffuseContribution;
			outSpecular += specularContribution;
			outSubsurface += subsurfaceContribution;
		}

		[loop] 
		for (uint lightIndex = 0; lightIndex < pointLightCount; ++lightIndex)
		{
			PointLight pointLight = GetPointLight(lightIndex);
			if (!pointLight.Enabled)
			{
				continue;
			}

			const float3 toLight = pointLight.Position - positionWorld;
			const float distanceToLight = length(toLight);
			const float attenuation = CalculatePointLightAttenuation(distanceToLight, pointLight.Radius);
			if (attenuation <= 0.0f)
			{
				continue;
			}

			const float3 pointLightDirection = toLight / max(distanceToLight, 0.001f);
			AccumulateDirectLight(
			    positionWorld,
			    viewDirWorld,
			    matProps,
			    pointLightDirection,
			    pointLight.Radiance * attenuation,
			    diffuseContribution,
			    specularContribution,
			    subsurfaceContribution);
			outDiffuse += diffuseContribution;
			outSpecular += specularContribution;
			outSubsurface += subsurfaceContribution;
		}
	}

	// =========================================================================
	// Indirect Lighting (IBL)
	// =========================================================================
	// Uses prefiltered environment maps for image-based lighting.
	//
	// Inputs:
	//   irradiance:     Diffuse irradiance (cosine-weighted hemisphere integral)
	//   prefilteredEnv: Specular radiance at reflection direction, mip-selected by roughness

	void CalculateIndirectIBL(
	    float3 viewDirWorld,
	    Material::Properties matProps,
	    float3 irradiance,
	    float3 prefilteredEnv,
	    out float3 outDiffuse,
	    out float3 outSpecular)
	{
		const float3 N = normalize(matProps.NormalWorld);
		const float3 V = normalize(viewDirWorld);
		const float NoV = saturate(dot(N, V));

		const float roughness = max(matProps.Roughness, 0.01f);
		const float metallic = matProps.Metallic;

		const float3 F0 = lerp(matProps.DielectricF0.xxx, matProps.BaseColor, metallic);

		BRDF::Indirect::Evaluate(
		    NoV,
		    matProps.BaseColor,
		    roughness,
		    metallic,
		    F0,
		    irradiance,
		    prefilteredEnv,
		    matProps.AmbientOcclusion,
		    outDiffuse,
		    outSpecular);
	}
}  // namespace Lighting

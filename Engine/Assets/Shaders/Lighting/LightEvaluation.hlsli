#pragma once

#include "Resources/ConstantBuffers.hlsli"
#include "Material/Material.hlsli"
#include "BRDF/BRDF.hlsli"
#include "Lighting/Shadow/ShadowEvaluation.hlsli"

namespace Lighting
{
	struct DirectionalLight
	{
		float3 Direction;
		float3 Radiance;
		bool CastShadow;
	};

	DirectionalLight GetDirectionalLight(uint lightIndex)
	{
		DirectionalLight light;
		light.Direction = normalize(-ViewLighting.DirectionalLights[lightIndex].Direction);
		light.Radiance = ViewLighting.DirectionalLights[lightIndex].Color * ViewLighting.DirectionalLights[lightIndex].Intensity;
		light.CastShadow = ViewLighting.DirectionalLights[lightIndex].CastShadow != 0u;
		return light;
	}

	void AccumulateDirectLight(
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

	void CalculateDirect(
	    float3 viewDirWorld,
	    float3 positionWorld,
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

		[loop] for (uint lightIndex = 0; lightIndex < directionalLightCount; ++lightIndex)
		{
			DirectionalLight light = GetDirectionalLight(lightIndex);
			const float shadowFactor = light.CastShadow
			    ? Shadow::ComputeShadowFactor(positionWorld, matProps.NormalWorld, light.Direction, lightIndex)
			    : 1.0f;

			AccumulateDirectLight(
			    viewDirWorld,
			    matProps,
			    light.Direction,
			    light.Radiance,
			    diffuseContribution,
			    specularContribution,
			    subsurfaceContribution);
			outDiffuse += diffuseContribution * shadowFactor;
			outSpecular += specularContribution * shadowFactor;
			outSubsurface += subsurfaceContribution * shadowFactor;
		}
	}

	float3 Evaluate(
	    PS::Input psInput,
	    Material::Properties matProps,
	    out float3 outDirectDiffuse,
	    out float3 outDirectSubsurface,
	    out float3 outDirectSpecular)
	{
		const float3 viewDir = normalize(Camera.Position - psInput.PositionWorld);
		CalculateDirect(viewDir, psInput.PositionWorld, matProps, outDirectDiffuse, outDirectSpecular, outDirectSubsurface);
		return outDirectDiffuse + outDirectSpecular + outDirectSubsurface + matProps.Emissive;
	}
}  // namespace Lighting

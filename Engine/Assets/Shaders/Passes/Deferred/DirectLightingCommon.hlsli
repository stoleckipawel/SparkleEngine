#ifndef SPARKLE_DIRECT_LIGHTING_COMMON_HLSLI
#define SPARKLE_DIRECT_LIGHTING_COMMON_HLSLI

#include "Resources/ConstantBuffers.hlsli"
#include "BRDF/BRDF.hlsli"

namespace DirectLighting
{
	void AccumulateDirectionalLight(
	    float3 viewDirWorld,
	    float3 normalWorld,
	    float roughness,
	    bool evaluateSubsurface,
	    uint lightIndex,
	    out float3 outDiffuse,
	    out float3 outSpecular,
	    out float3 outSubsurface)
	{
		const float3 lightDirection = normalize(-ViewLighting.DirectionalLights[lightIndex].Direction);
		BRDF::ShadingData shadingData = BRDF::ComputeShadingData(normalWorld, viewDirWorld, lightDirection);

		if (shadingData.NoL <= 0.0f || shadingData.NoV <= 0.0f)
		{
			outDiffuse = 0.0f;
			outSpecular = 0.0f;
			outSubsurface = 0.0f;
			return;
		}

		const float3 radiance = ViewLighting.DirectionalLights[lightIndex].Color * ViewLighting.DirectionalLights[lightIndex].Intensity;
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
}

#endif
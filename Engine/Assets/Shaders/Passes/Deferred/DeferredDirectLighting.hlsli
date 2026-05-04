#ifndef SPARKLE_DEFERRED_DIRECT_LIGHTING_HLSLI
#define SPARKLE_DEFERRED_DIRECT_LIGHTING_HLSLI

#include "Resources/ConstantBuffers.hlsli"
#include "BRDF/BRDF.hlsli"

namespace DeferredDirectLighting
{
	void AccumulateDirectionalLight(
		float3 positionWorld,
		float3 viewDirWorld,
		float3 normalWorld,
		float3 baseColor,
		float roughness,
		float metallic,
		float3 subsurfaceColor,
		float subsurfaceStrength,
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

		const float3 radiance = ViewLighting.DirectionalLights[lightIndex].Color *
		    ViewLighting.DirectionalLights[lightIndex].Intensity;
		const float3 dielectricF0 = float3(0.04f, 0.04f, 0.04f);
		const float3 f0 = lerp(dielectricF0, baseColor, saturate(metallic));

		float3 directSubsurface;
		BRDF::Direct::Evaluate(
		    shadingData,
		    baseColor,
		    max(roughness, 0.04f),
		    saturate(metallic),
		    f0,
		    subsurfaceColor,
		    subsurfaceStrength,
		    outDiffuse,
		    outSpecular,
		    outSubsurface);

		outDiffuse *= radiance * shadingData.NoL;
		outSpecular *= radiance * shadingData.NoL;
		outSubsurface *= radiance * shadingData.NoL;
	}
}  // namespace DeferredDirectLighting

#endif
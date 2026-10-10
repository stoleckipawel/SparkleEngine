#ifndef SPARKLE_SURFACE_LIGHTING_HLSLI
#define SPARKLE_SURFACE_LIGHTING_HLSLI

#include "/Engine/BRDF/BRDF.hlsli"
#include "/Engine/Lighting/LightSampling.hlsli"

namespace SurfaceLighting
{
	float3 BuildF0(float3 baseColor, float metallic, float dielectricF0)
	{
		return lerp(saturate(dielectricF0).xxx, saturate(baseColor), saturate(metallic));
	}

	void EvaluateDirectLightWithF0(float3 viewDirWorld,
	                               float3 normalWorld,
	                               float3 baseColor,
	                               float roughness,
	                               float metallic,
	                               float3 f0,
	                               float3 subsurfaceColor,
	                               float subsurfaceStrength,
	                               bool evaluateDiffuse,
	                               bool evaluateSpecular,
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

		const BRDF::Direct::Response
		    response = BRDF::Direct::Evaluate(shadingData, baseColor, roughness, metallic, f0, subsurfaceColor, subsurfaceStrength, evaluateDiffuse, evaluateSpecular, evaluateSubsurface);

		const float3 incidentRadiance = radiance * shadingData.NoL;
		outDiffuse = response.Diffuse * incidentRadiance;
		outSpecular = response.Specular * incidentRadiance;
		outSubsurface = response.Subsurface * incidentRadiance;
	}

	void EvaluateDirectLight(float3 viewDirWorld,
	                         float3 normalWorld,
	                         float3 baseColor,
	                         float roughness,
	                         float metallic,
	                         float dielectricF0,
	                         float3 subsurfaceColor,
	                         float subsurfaceStrength,
	                         bool evaluateDiffuse,
	                         bool evaluateSpecular,
	                         bool evaluateSubsurface,
	                         float3 lightDirection,
	                         float3 radiance,
	                         out float3 outDiffuse,
	                         out float3 outSpecular,
	                         out float3 outSubsurface)
	{
		const float3 f0 = BuildF0(baseColor, metallic, dielectricF0);

		EvaluateDirectLightWithF0(viewDirWorld,
		                          normalWorld,
		                          baseColor,
		                          roughness,
		                          metallic,
		                          f0,
		                          subsurfaceColor,
		                          subsurfaceStrength,
		                          evaluateDiffuse,
		                          evaluateSpecular,
		                          evaluateSubsurface,
		                          lightDirection,
		                          radiance,
		                          outDiffuse,
		                          outSpecular,
		                          outSubsurface);
	}

	void AccumulateDirectLightSample(float3 viewDirWorld,
	                                 float3 normalWorld,
	                                 float3 baseColor,
	                                 float roughness,
	                                 float metallic,
	                                 float dielectricF0,
	                                 float3 subsurfaceColor,
	                                 float subsurfaceStrength,
	                                 bool evaluateDiffuse,
	                                 bool evaluateSpecular,
	                                 bool evaluateSubsurface,
	                                 LightSampling::DirectLightSample lightSample,
	                                 float shadowVisibility,
	                                 out float3 outDiffuse,
	                                 out float3 outSpecular,
	                                 out float3 outSubsurface)
	{
		if (!lightSample.Valid || shadowVisibility <= 0.0f)
		{
			outDiffuse = 0.0f.xxx;
			outSpecular = 0.0f.xxx;
			outSubsurface = 0.0f.xxx;
			return;
		}

		const float samplePdf = lightSample.PdfW * lightSample.LightSelectionPdf;
		if (samplePdf <= 0.0f)
		{
			outDiffuse = 0.0f.xxx;
			outSpecular = 0.0f.xxx;
			outSubsurface = 0.0f.xxx;
			return;
		}

		EvaluateDirectLight(viewDirWorld,
		                    normalWorld,
		                    baseColor,
		                    roughness,
		                    metallic,
		                    dielectricF0,
		                    subsurfaceColor,
		                    subsurfaceStrength,
		                    evaluateDiffuse,
		                    evaluateSpecular,
		                    evaluateSubsurface,
		                    lightSample.DirectionWorld,
		                    lightSample.IncidentRadiance * shadowVisibility / samplePdf,
		                    outDiffuse,
		                    outSpecular,
		                    outSubsurface);
	}
}

#endif

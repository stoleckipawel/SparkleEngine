#ifndef SPARKLE_REFERENCE_PATH_TRACER_DIRECT_LIGHTING_HLSLI
#define SPARKLE_REFERENCE_PATH_TRACER_DIRECT_LIGHTING_HLSLI

#include "/Engine/RayTracing/PathBsdf.hlsli"
#include "/Engine/RayTracing/PathTracer.hlsli"
#include "/Engine/RayTracing/PathVisibility.hlsli"
#include "/Engine/Passes/Lighting/ReferencePathTracer/ReferencePathTracerLightSampling.hlsli"
#include "/Engine/Passes/Lighting/ReferencePathTracer/ReferencePathTracerSampler.hlsli"

namespace ReferencePathTracer
{
	struct PreviousPathEvent
	{
		float3 PositionWorld;
		float BsdfPdfW;
		bool Delta;
	};

	float3 EvaluateEnvironmentRadiance(LightCounts lightCounts,
	                                   PreviousPathEvent previousEvent,
	                                   float3 directionWorld,
	                                   Texture2D skyTexture,
	                                   SamplerState skySampler)
	{
		const float lightPdfW = EnvironmentPdfW(lightCounts);
		const float misWeight = previousEvent.Delta ? 1.0f : PathTracer::PowerHeuristic(previousEvent.BsdfPdfW, lightPdfW);
		return SampleSkyRadiance(skyTexture, skySampler, directionWorld) * misWeight;
	}

	float3 EvaluateSurfaceEmission(LightCounts lightCounts, PreviousPathEvent previousEvent, RayTracingPathSurface surface)
	{
		if (previousEvent.Delta || !any(surface.EmissiveColor > 0.0f))
		{
			return surface.EmissiveColor;
		}
		const float lightPdfW = EmissiveTrianglePdfW(lightCounts, previousEvent.PositionWorld, surface);
		return surface.EmissiveColor * PathTracer::PowerHeuristic(previousEvent.BsdfPdfW, lightPdfW);
	}

	float3 SampleDirectLighting(RaytracingAccelerationStructure sceneTlas,
	                            Texture2D skyTexture,
	                            SamplerState skySampler,
	                            RayTracingPathSurface surface,
	                            PathBsdf::LobeMasses lobeMasses,
	                            LightCounts lightCounts,
	                            SampleIdentity sampleIdentity,
	                            uint surfaceDepth)
	{
		if (lightCounts.Total == 0u || (lobeMasses.Diffuse == 0.0f && (lobeMasses.Specular == 0.0f || lobeMasses.SpecularDelta)))
		{
			return 0.0f.xxx;
		}

		const uint lightChoiceDimension = SurfaceDimension(surfaceDepth, LightChoiceOffset);
		const LightSelection selection = SelectLight(lightCounts, RandomWord(sampleIdentity, lightChoiceDimension));
		const float2 lightShapeSample =
		    float2(CommonRandom::OpenUnitInterval(RandomWord(sampleIdentity, SurfaceDimension(surfaceDepth, LightShapeXOffset))),
		           CommonRandom::OpenUnitInterval(RandomWord(sampleIdentity, SurfaceDimension(surfaceDepth, LightShapeYOffset))));
		const LightSampling::DirectLightSample light =
		    SampleLight(selection, surface.PositionWorld, lightShapeSample, skyTexture, skySampler);
		const PathBsdf::Evaluation bsdf = PathBsdf::EvaluateContinuous(surface, light.DirectionWorld, lobeMasses);
		if (!bsdf.HasSupport || !PathVisibility::IsUnoccluded(sceneTlas, surface, light))
		{
			return 0.0f.xxx;
		}

		const float lightProbability = light.LightSelectionPdf * (light.Delta ? 1.0f : light.PdfW);
		const float misWeight = light.Delta ? 1.0f : PathTracer::PowerHeuristic(lightProbability, bsdf.PdfW);
		return bsdf.F * light.IncidentRadiance * bsdf.Cosine * misWeight / lightProbability;
	}
}

#endif

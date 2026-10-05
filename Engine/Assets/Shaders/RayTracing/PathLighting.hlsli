#ifndef SPARKLE_RAY_TRACING_PATH_LIGHTING_HLSLI
#define SPARKLE_RAY_TRACING_PATH_LIGHTING_HLSLI

#include "/Engine/Resources/FrameUniformData.hlsli"

#include "/Engine/Lighting/Sky.hlsli"
#include "/Engine/RayTracing/PathTracer.hlsli"
#include "/Engine/RayTracing/PathSampling.hlsli"
#include "/Engine/RayTracing/PathTrace.hlsli"
#include "/Engine/RayTracing/RayTracingHitLighting.hlsli"

namespace RayTracingPathLighting
{
	struct Result
	{
		float3 DiffuseContribution;
		float3 SpecularContribution;
		RayTracingPathSample::LightingResult FirstLighting;
	};

	RayTracingPathSample::LightingResult ResolveLighting(RayTracingTraceResult trace,
	                                                     float3 directionWorld,
	                                                     Texture2D skyTexture,
	                                                     SamplerState skySampler,
	                                                     uint pathSampleIndex,
	                                                     uint bounceIndex,
	                                                     uint randomFrameIndex,
	                                                     bool traceSecondaryShadows,
	                                                     out RayTracingHitSurfaceData outHitSurface)
	{
		outHitSurface = (RayTracingHitSurfaceData)0;

		RayTracingPathSample::LightingResult result = (RayTracingPathSample::LightingResult)0;
		result.Hit = trace.Hit;

		if (trace.Hit)
		{
			const RayTracingHitSurfaceData hitSurface = ReconstructRayTracingHitSurface(trace, directionWorld);
			outHitSurface = hitSurface;
			result.Hit = hitSurface.Valid;
			result.HitPositionWorld = hitSurface.Valid ? hitSurface.PositionWorld : 0.0f.xxx;
			result.IncidentRadiance = hitSurface.Valid ? ShadeRayTracingHitIncidentRadiance(hitSurface,
			                                                                                directionWorld,
			                                                                                pathSampleIndex,
			                                                                                bounceIndex,
			                                                                                randomFrameIndex,
			                                                                                traceSecondaryShadows)
			                                           : 0.0f.xxx;
			return result;
		}

		result.IncidentRadiance = SampleSkyRadiance(skyTexture, skySampler, directionWorld);
		return result;
	}

	Result TraceSurfacePathWithRandomFrame(Texture2D skyTexture,
	                                       SamplerState skySampler,
	                                       RayTracingPathSurface primarySurface,
	                                       uint2 pixelCoord,
	                                       uint sampleIndex,
	                                       uint specularSampleMode,
	                                       uint bounceCount,
	                                       uint randomFrameIndex,
	                                       bool evaluatePrimaryDiffuse,
	                                       bool evaluatePrimarySpecular,
	                                       bool traceSecondaryShadows)
	{
		Result result = (Result)0;

		RayTracingPathSurface surface = primarySurface;
		PathTracer::PathState path = (PathTracer::PathState)0;
		path.Throughput = 1.0f.xxx;
		float3 diffuseThroughput = 0.0f.xxx;
		float3 specularThroughput = 0.0f.xxx;
		const uint sanitizedBounceCount = max(bounceCount, 1u);

		[loop]
		for (uint bounceIndex = 0u; bounceIndex < sanitizedBounceCount; ++bounceIndex)
		{
			const RayTracingPathSampling::RandomSamples randomSamples =
			    RayTracingPathSampling::GenerateRandomSamples(pixelCoord, bounceIndex, sampleIndex, randomFrameIndex);
			const RayTracingPathSample::DirectionSample sample =
			    RayTracingPathSampling::SampleBSDF(surface,
			                                       specularSampleMode,
			                                       randomSamples,
			                                       bounceIndex != 0u || evaluatePrimaryDiffuse,
			                                       bounceIndex != 0u || evaluatePrimarySpecular);
			if (!sample.HasSupport)
			{
				break;
			}

			PathTracer::ApplyDirectionSample(path, sample);
			if (bounceIndex == 0u)
			{
				diffuseThroughput = sample.DiffuseThroughput;
				specularThroughput = sample.SpecularThroughput;
			}
			else
			{
				const float3 continuationThroughput = sample.DiffuseThroughput + sample.SpecularThroughput;
				if (evaluatePrimaryDiffuse)
				{
					diffuseThroughput *= continuationThroughput;
				}
				if (evaluatePrimarySpecular)
				{
					specularThroughput *= continuationThroughput;
				}
			}

			const float survivalProbability = RayTracingPathSampling::RussianRouletteSurvivalProbability(path.Throughput, bounceIndex);
			if (survivalProbability <= 0.0f || randomSamples.Roulette > survivalProbability)
			{
				break;
			}
			if (survivalProbability < 1.0f)
			{
				PathTracer::ApplySurvivalCompensation(path.Throughput, survivalProbability);
				if (evaluatePrimaryDiffuse)
				{
					PathTracer::ApplySurvivalCompensation(diffuseThroughput, survivalProbability);
				}
				if (evaluatePrimarySpecular)
				{
					PathTracer::ApplySurvivalCompensation(specularThroughput, survivalProbability);
				}
			}

			float3 rayOriginWorld = 0.0f.xxx;
			const RayTracingTraceResult trace = RayTracingPathTrace::TraceSurfaceRay(surface, path.DirectionWorld, rayOriginWorld);
			path.OriginWorld = rayOriginWorld;
			RayTracingHitSurfaceData hitSurface;
			RayTracingPathSample::LightingResult lighting = ResolveLighting(trace,
			                                                                sample.DirectionWorld,
			                                                                skyTexture,
			                                                                skySampler,
			                                                                sampleIndex,
			                                                                bounceIndex,
			                                                                randomFrameIndex,
			                                                                traceSecondaryShadows,
			                                                                hitSurface);
			if (evaluatePrimaryDiffuse)
			{
				PathTracer::AddRadiance(result.DiffuseContribution, diffuseThroughput, lighting.IncidentRadiance);
			}
			if (evaluatePrimarySpecular)
			{
				PathTracer::AddRadiance(result.SpecularContribution, specularThroughput, lighting.IncidentRadiance);
			}

			if (bounceIndex == 0u)
			{
				result.FirstLighting = lighting;
			}
			if (!lighting.Hit)
			{
				break;
			}

			surface = BuildHitRayTracingPathSurface(hitSurface, path.DirectionWorld);
		}

		return result;
	}

}

#endif

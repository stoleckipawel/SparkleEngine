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
		uint PrimaryLobe;
		float3 FinalContribution;
		RayTracingPathSample::LightingResult FirstLighting;
	};

	RayTracingPathSample::LightingResult ResolveLighting(RayTracingTraceResult trace,
	                                                     float3 directionWorld,
	                                                     Texture2D skyTexture,
	                                                     SamplerState skySampler,
	                                                     uint pathSampleIndex,
	                                                     uint bounceIndex,
	                                                     uint randomFrameIndex,
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
			result.IncidentRadiance = hitSurface.Valid
			    ? ShadeRayTracingHitIncidentRadiance(hitSurface, directionWorld, pathSampleIndex, bounceIndex, randomFrameIndex)
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
	                                       uint randomFrameIndex)
	{
		Result result = (Result)0;
		result.PrimaryLobe = RayTracingPathSample::LobeNone;

		RayTracingPathSurface surface = primarySurface;
		PathTracer::PathState path = (PathTracer::PathState)0;
		path.Throughput = 1.0f.xxx;
		const uint sanitizedBounceCount = max(bounceCount, 1u);

		[loop] for (uint bounceIndex = 0u; bounceIndex < sanitizedBounceCount; ++bounceIndex)
		{
			const RayTracingPathSampling::RandomSamples randomSamples =
			    RayTracingPathSampling::GenerateRandomSamples(pixelCoord, bounceIndex, sampleIndex, randomFrameIndex);
			const RayTracingPathSample::DirectionSample sample =
			    RayTracingPathSampling::SampleBSDF(surface, specularSampleMode, randomSamples);
			if (bounceIndex == 0u)
			{
				result.PrimaryLobe = sample.Lobe;
			}
			if (!sample.HasSupport)
			{
				break;
			}

			PathTracer::ApplyDirectionSample(path, sample);
			if (!RayTracingPathSampling::SurvivesRussianRoulette(path.Throughput, randomSamples.Roulette, bounceIndex))
			{
				break;
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
			                                                                hitSurface);
			PathTracer::AddRadiance(result.FinalContribution, path.Throughput, lighting.IncidentRadiance);

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

	Result TraceSurfacePath(Texture2D skyTexture,
	                        SamplerState skySampler,
	                        RayTracingPathSurface primarySurface,
	                        uint2 pixelCoord,
	                        uint sampleIndex,
	                        uint specularSampleMode,
	                        uint bounceCount)
	{
		return TraceSurfacePathWithRandomFrame(skyTexture,
		                                       skySampler,
		                                       primarySurface,
		                                       pixelCoord,
		                                       sampleIndex,
		                                       specularSampleMode,
		                                       bounceCount,
		                                       FrameIndex);
	}
}

#endif

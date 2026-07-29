#ifndef SPARKLE_RAY_TRACING_PATH_LIGHTING_HLSLI
#define SPARKLE_RAY_TRACING_PATH_LIGHTING_HLSLI

#include "Lighting/Sky.hlsli"
#include "RayTracing/PathSampling.hlsli"
#include "RayTracing/PathTrace.hlsli"
#include "RayTracing/RayTracingHitLighting.hlsli"

namespace RayTracingPathLighting
{
	struct Result
	{
		uint PrimaryLobe;
		float3 FinalContribution;
		RayTracingPathSample::DirectionSample FirstSample;
		RayTracingPathSample::LightingResult FirstLighting;
		RayTracingTraceResult FirstTrace;
		RayTracingHitSurfaceData FirstHitSurface;
	};

	RayTracingPathSample::LightingResult ResolveLighting(
	    RayTracingTraceResult trace,
	    RayTracingPathSample::DirectionSample sample,
	    float3 rayOriginWorld,
	    Texture2D skyTexture,
	    SamplerState skySampler,
	    uint pathSampleIndex,
	    uint bounceIndex,
	    uint randomFrameIndex,
	    out RayTracingHitSurfaceData outHitSurface)
	{
		outHitSurface = (RayTracingHitSurfaceData) 0;

		RayTracingPathSample::LightingResult result;
		result.TraceHit = trace.Hit;
		result.Hit = trace.Hit;
		result.HitDistance = trace.RayT;
		result.RejectionReason = sample.RejectionReason;
		result.IncidentRadiance = 0.0f.xxx;
		result.Contribution = 0.0f.xxx;
		result.HitPositionWorld = 0.0f.xxx;
		result.HitNormalWorld = 0.0f.xxx;
		result.MaterialBaseColor = 0.0f.xxx;
		result.MissRadiance = 0.0f.xxx;
		result.SurfaceRejectionReason = trace.Hit ? RayTracingHitSurface::ReasonInvalidHitData : RayTracingHitSurface::ReasonNoHit;

		if (sample.RejectionReason != RayTracingPathSample::RejectionReasonNone)
		{
			return result;
		}

		if (trace.Hit)
		{
			const RayTracingHitSurfaceData hitSurface = ReconstructRayTracingHitSurface(trace, rayOriginWorld, sample.DirectionWorld);
			outHitSurface = hitSurface;
			result.Hit = hitSurface.Valid;
			result.RejectionReason =
			    hitSurface.Valid ? RayTracingPathSample::RejectionReasonNone : RayTracingPathSample::RejectionReasonHitSurfaceRejected;
			result.SurfaceRejectionReason = hitSurface.RejectionReason;
			result.HitPositionWorld = hitSurface.Valid ? hitSurface.PositionWorld : 0.0f.xxx;
			result.HitNormalWorld = hitSurface.Valid ? hitSurface.NormalWorld : 0.0f.xxx;
			result.MaterialBaseColor = hitSurface.Valid ? hitSurface.BaseColor : 0.0f.xxx;
			result.IncidentRadiance =
			    hitSurface.Valid
			        ? ShadeRayTracingHitIncidentRadiance(hitSurface, sample.DirectionWorld, pathSampleIndex, bounceIndex, randomFrameIndex)
			        : 0.0f.xxx;
			return result;
		}

		result.RejectionReason = RayTracingPathSample::RejectionReasonTraceMiss;
		result.SurfaceRejectionReason = RayTracingHitSurface::ReasonNoHit;
		result.MissRadiance = SampleSkyRadiance(skyTexture, skySampler, sample.DirectionWorld);
		result.IncidentRadiance = result.MissRadiance;
		return result;
	}

	Result TraceSurfacePathWithRandomFrame(
	    Texture2D skyTexture,
	    SamplerState skySampler,
	    RayTracingPathSurface primarySurface,
	    uint2 pixelCoord,
	    uint sampleIndex,
	    uint specularSampleMode,
	    uint bounceCount,
	    uint randomFrameIndex,
	    RayTracingPathTrace::TraceSettings traceSettings)
	{
		Result result = (Result) 0;
		result.PrimaryLobe = RayTracingPathSample::LobeNone;
		result.FirstSample = RayTracingPathSampling::InvalidSample(RayTracingPathSample::LobeNone);
		result.FirstLighting = (RayTracingPathSample::LightingResult) 0;
		result.FirstLighting.SurfaceRejectionReason = RayTracingHitSurface::ReasonNoHit;
		result.FirstLighting.RejectionReason = RayTracingPathSample::RejectionReasonTraceMiss;
		result.FirstTrace = (RayTracingTraceResult) 0;
		result.FirstHitSurface = (RayTracingHitSurfaceData) 0;

		RayTracingPathSurface surface = primarySurface;
		float3 throughput = 1.0f.xxx;
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
				result.FirstSample = sample;
			}
			if (sample.RejectionReason != RayTracingPathSample::RejectionReasonNone)
			{
				break;
			}

			throughput *= sample.Throughput;
			if (max(max(throughput.r, throughput.g), throughput.b) <= 0.0f)
			{
				break;
			}
			if (!RayTracingPathSampling::SurvivesRussianRoulette(throughput, randomSamples.Roulette, bounceIndex))
			{
				break;
			}

			float3 rayOriginWorld = 0.0f.xxx;
			const RayTracingTraceResult trace =
			    RayTracingPathTrace::TraceSurfaceRay(surface, sample.DirectionWorld, traceSettings, rayOriginWorld);
			RayTracingHitSurfaceData hitSurface;
			RayTracingPathSample::LightingResult lighting = ResolveLighting(
			    trace,
			    sample,
			    rayOriginWorld,
			    skyTexture,
			    skySampler,
			    sampleIndex,
			    bounceIndex,
			    randomFrameIndex,
			    hitSurface);
			lighting.Contribution = lighting.IncidentRadiance * throughput;
			result.FinalContribution += lighting.Contribution;

			if (bounceIndex == 0u)
			{
				result.FirstLighting = lighting;
				result.FirstTrace = trace;
				result.FirstHitSurface = hitSurface;
			}
			if (!lighting.Hit)
			{
				break;
			}

			surface = BuildHitRayTracingPathSurface(hitSurface, sample.DirectionWorld);
		}

		return result;
	}

	Result TraceSurfacePath(
	    Texture2D skyTexture,
	    SamplerState skySampler,
	    RayTracingPathSurface primarySurface,
	    uint2 pixelCoord,
	    uint sampleIndex,
	    uint specularSampleMode,
	    uint bounceCount,
	    RayTracingPathTrace::TraceSettings traceSettings)
	{
		return TraceSurfacePathWithRandomFrame(
		    skyTexture,
		    skySampler,
		    primarySurface,
		    pixelCoord,
		    sampleIndex,
		    specularSampleMode,
		    bounceCount,
		    FrameIndex,
		    traceSettings);
	}
}

#endif

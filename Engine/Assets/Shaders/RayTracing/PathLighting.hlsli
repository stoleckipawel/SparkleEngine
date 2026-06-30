#ifndef SPARKLE_RAY_TRACING_PATH_LIGHTING_HLSLI
#define SPARKLE_RAY_TRACING_PATH_LIGHTING_HLSLI

#include "Lighting/SkyEnvironment.hlsli"
#include "RayTracing/PathSampling.hlsli"
#include "RayTracing/RayTracingHitLighting.hlsli"
#include "RayTracing/RayTracingTraceQuery.hlsli"

namespace RayTracingPathLighting
{
	struct TraceSettings
	{
		float NormalBias;
		float MaxDistance;
		float MinT;
		uint RayFlags;
		uint InstanceMask;
	};

	struct Result
	{
		uint PrimaryLobe;
		float3 FinalContribution;
		RayTracingPathSample::DirectionSample FirstSample;
		RayTracingPathSample::LightingResult FirstLighting;
		RayTracingTraceResult FirstTrace;
		RayTracingHitSurfaceData FirstHitSurface;
	};

	float3 ComputeRayOrigin(RayTracingPathSurface surface, float3 rayDirectionWorld, TraceSettings settings)
	{
		const float bias = max(settings.NormalBias, 0.0f);
		const float NoR = abs(dot(surface.NormalWorld, rayDirectionWorld));
		const float grazingScale = rcp(max(NoR, 0.25f));
		return surface.PositionWorld + surface.NormalWorld * bias * grazingScale + rayDirectionWorld * settings.MinT;
	}

	RayTracingPathSample::LightingResult ResolveLighting(
	    RayTracingTraceResult trace,
	    RayTracingPathSample::DirectionSample sample,
	    float3 rayOriginWorld,
	    Texture2D skyTexture,
	    SamplerState skySampler,
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
		result.SurfaceRejectionReason = trace.Hit ? RayTracingHitSurface::ReasonHitDataUnavailable : RayTracingHitSurface::ReasonNoHit;

		if (sample.RejectionReason != RayTracingPathSample::RejectionReasonNone)
		{
			return result;
		}

		if (trace.Hit)
		{
			const RayTracingHitSurfaceData hitSurface =
			    ReconstructRayTracingHitSurface(trace, rayOriginWorld, sample.DirectionWorld);
			outHitSurface = hitSurface;
			result.Hit = hitSurface.Valid;
			result.RejectionReason = hitSurface.Valid
			                             ? RayTracingPathSample::RejectionReasonNone
			                             : RayTracingPathSample::RejectionReasonHitSurfaceRejected;
			result.SurfaceRejectionReason = hitSurface.RejectionReason;
			result.HitPositionWorld = hitSurface.Valid ? hitSurface.PositionWorld : 0.0f.xxx;
			result.HitNormalWorld = hitSurface.Valid ? hitSurface.NormalWorld : 0.0f.xxx;
			result.MaterialBaseColor = hitSurface.Valid ? hitSurface.BaseColor : 0.0f.xxx;
			result.IncidentRadiance = hitSurface.Valid
			                              ? ShadeRayTracingHitIncidentRadiance(hitSurface, sample.DirectionWorld)
			                              : 0.0f.xxx;
			return result;
		}

		result.RejectionReason = RayTracingPathSample::RejectionReasonTraceMiss;
		result.SurfaceRejectionReason = RayTracingHitSurface::ReasonNoHit;
		result.MissRadiance = SampleSkyEnvironmentRadiance(skyTexture, skySampler, sample.DirectionWorld);
		result.IncidentRadiance = result.MissRadiance;
		return result;
	}

	Result TraceSurfacePath(
	    RaytracingAccelerationStructure sceneTlas,
	    Texture2D skyTexture,
	    SamplerState skySampler,
	    RayTracingPathSurface primarySurface,
	    uint2 pixelCoord,
	    uint specularSampleMode,
	    uint bounceCount,
	    TraceSettings traceSettings)
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
			    RayTracingPathSampling::GenerateRandomSamples(pixelCoord, bounceIndex);
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
			if (max(max(throughput.r, throughput.g), throughput.b) <= 1.0e-4f)
			{
				break;
			}
			if (!RayTracingPathSampling::SurvivesRussianRoulette(throughput, randomSamples.Roulette, bounceIndex))
			{
				break;
			}

			const float3 rayOriginWorld = ComputeRayOrigin(surface, sample.DirectionWorld, traceSettings);
			const RayTracingTraceResult trace =
			    TraceRayQueryWithAlphaTest(
			        sceneTlas,
			        rayOriginWorld,
			        sample.DirectionWorld,
			        traceSettings.MinT,
			        max(traceSettings.MaxDistance, traceSettings.MinT),
			        traceSettings.RayFlags,
			        traceSettings.InstanceMask);
			RayTracingHitSurfaceData hitSurface;
			RayTracingPathSample::LightingResult lighting =
			    ResolveLighting(trace, sample, rayOriginWorld, skyTexture, skySampler, hitSurface);
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
}

#endif

#include "/Engine/Lighting/Sky.hlsli"
#include "/Engine/Resources/ViewCameraRay.hlsli"

#include "/Engine/RayTracing/RayTracingHitUniformData.hlsli"
#include "/Engine/RayTracing/PathTracer.hlsli"
#include "/Engine/RayTracing/RayTracingHitPathSurface.hlsli"
#include "/Engine/RayTracing/RayTracingTraceQuery.hlsli"
#include "/Engine/Passes/Lighting/ReferencePathTracer/ReferencePathTracerSampler.hlsli"

RWTexture2D<float4> SceneColor;
RaytracingAccelerationStructure SceneTlas;
Texture2D SkyTexture;
SamplerState SamplerLinearClamp;

cbuffer ReferencePathTracerConstants
{
	uint SessionSeed;
	uint ReplicateId;
	uint SampleOrdinal;
	uint MaximumSurfaceVertices;
	float ContinuationNormalBiasMeters;
	uint3 ReferencePathTracerPadding;
};

[numthreads(8, 8, 1)] void main(uint3 dispatchThreadId : SV_DispatchThreadID)
{
	uint width = 0u;
	uint height = 0u;
	SceneColor.GetDimensions(width, height);
	const uint2 pixelCoord = dispatchThreadId.xy;
	if (pixelCoord.x >= width || pixelCoord.y >= height)
	{
		return;
	}

	float3 contribution = 0.0f.xxx;
	bool complete = false;
	if (MaximumSurfaceVertices == 0u
	    || !ReferencePathTracerSampler::IsValidIdentity(pixelCoord, SampleOrdinal, ReferencePathTracerSampler::FilmY))
	{
		SceneColor[pixelCoord] = PathTracer::InvalidRadiance();
		return;
	}

	const float2 filmSample = float2(
	    CommonRandom::OpenUnitInterval(
	        ReferencePathTracerSampler::Word(pixelCoord, SampleOrdinal, ReferencePathTracerSampler::FilmX, SessionSeed, ReplicateId)),
	    CommonRandom::OpenUnitInterval(
	        ReferencePathTracerSampler::Word(pixelCoord, SampleOrdinal, ReferencePathTracerSampler::FilmY, SessionSeed, ReplicateId)));
	const ViewCameraRay cameraRay = BuildPerspectiveViewCameraRay(pixelCoord, uint2(width, height), filmSample);
	if (!cameraRay.Valid)
	{
		SceneColor[pixelCoord] = PathTracer::InvalidRadiance();
		return;
	}

	PathTracer::PathState path;
	path.OriginWorld = cameraRay.OriginWorld;
	path.DirectionWorld = cameraRay.DirectionWorld;
	path.Throughput = 1.0.xxx;
	path.SurfaceDepth = 0u;
	float traversalTMin = cameraRay.TMin;
	float traversalTMax = cameraRay.TMax;

	[loop] for (;;)
	{
		const RayTracingTraceResult trace = TraceOpaqueRayQuery(SceneTlas,
		                                                        path.OriginWorld,
		                                                        path.DirectionWorld,
		                                                        traversalTMin,
		                                                        traversalTMax,
		                                                        RAY_FLAG_SKIP_CLOSEST_HIT_SHADER,
		                                                        0xFFu);
		if (!trace.Hit)
		{
			const float3 environmentRadiance = SampleSkyRadiance(SkyTexture, SamplerLinearClamp, path.DirectionWorld);
			if (!PathTracer::TryAddRadiance(contribution, path.Throughput, environmentRadiance))
			{
				SceneColor[pixelCoord] = PathTracer::InvalidRadiance();
				return;
			}
			complete = true;
			break;
		}

		const RayTracingPathSurface surface = BuildStaticOpaqueLambertianPathSurface(trace, -path.DirectionWorld);
		if (!surface.Valid)
		{
			SceneColor[pixelCoord] = PathTracer::InvalidRadiance();
			return;
		}
		if (!PathTracer::TryAddRadiance(contribution, path.Throughput, surface.EmissiveColor))
		{
			SceneColor[pixelCoord] = PathTracer::InvalidRadiance();
			return;
		}

		if (path.SurfaceDepth + 1u == MaximumSurfaceVertices)
		{
			complete = true;
			break;
		}

		const uint bsdfXDimension =
		    ReferencePathTracerSampler::SurfaceDimension(path.SurfaceDepth, ReferencePathTracerSampler::BsdfDirectionXOffset);
		const uint bsdfYDimension =
		    ReferencePathTracerSampler::SurfaceDimension(path.SurfaceDepth, ReferencePathTracerSampler::BsdfDirectionYOffset);
		if (!ReferencePathTracerSampler::IsValidIdentity(pixelCoord, SampleOrdinal, bsdfYDimension))
		{
			SceneColor[pixelCoord] = PathTracer::InvalidRadiance();
			return;
		}

		const float2 bsdfSample =
		    float2(CommonRandom::OpenUnitInterval(
		               ReferencePathTracerSampler::Word(pixelCoord, SampleOrdinal, bsdfXDimension, SessionSeed, ReplicateId)),
		           CommonRandom::OpenUnitInterval(
		               ReferencePathTracerSampler::Word(pixelCoord, SampleOrdinal, bsdfYDimension, SessionSeed, ReplicateId)));
		const float lobeSelectionMass = 1.0f;
		const RayTracingPathSample::DirectionSample bsdf = PathTracer::SampleLambertian(surface, bsdfSample, lobeSelectionMass);
		if (bsdf.RejectionReason != RayTracingPathSample::RejectionReasonNone)
		{
			SceneColor[pixelCoord] = PathTracer::InvalidRadiance();
			return;
		}
		if (!bsdf.HasSupport)
		{
			complete = true;
			break;
		}
		if (!PathTracer::ApplyDirectionSample(path, bsdf))
		{
			SceneColor[pixelCoord] = PathTracer::InvalidRadiance();
			return;
		}

		path.OriginWorld = surface.PositionWorld + surface.NormalWorld * ContinuationNormalBiasMeters;
		traversalTMin = 0.0f;
		traversalTMax = 3.402823466e+38f;
	}

	SceneColor[pixelCoord] =
	    complete && PathTracer::IsFiniteNonNegative(contribution) ? float4(contribution, 1.0f) : PathTracer::InvalidRadiance();
}

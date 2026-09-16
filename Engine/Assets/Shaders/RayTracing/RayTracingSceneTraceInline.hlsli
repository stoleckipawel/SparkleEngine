#ifndef SPARKLE_RAY_TRACING_SCENE_TRACE_INLINE_HLSLI
#define SPARKLE_RAY_TRACING_SCENE_TRACE_INLINE_HLSLI

#include "/Engine/RayTracing/RayTracingMaterialAlpha.hlsli"
#include "/Engine/RayTracing/RayTracingSceneTrace.hlsli"

RayTracingTraceResult TraceSceneRay(RaytracingAccelerationStructure sceneTlas,
                                    float3 originWorld,
                                    float3 directionWorld,
                                    float tMin,
                                    float tMax,
                                    uint rayFlags,
                                    uint instanceMask)
{
	RayDesc ray;
	ray.Direction = directionWorld;
	ray.Origin = originWorld;
	ray.TMin = tMin;
	ray.TMax = tMax;

	RayQuery<RAY_FLAG_NONE> query;
	query.TraceRayInline(sceneTlas, rayFlags | RAY_FLAG_SKIP_CLOSEST_HIT_SHADER, instanceMask, ray);
	while (query.Proceed())
	{
		if (query.CandidateType() == CANDIDATE_NON_OPAQUE_TRIANGLE
		    && ResolveRayTracingCandidateAlpha(query.CandidateInstanceID(),
	                                        query.CandidatePrimitiveIndex(),
	                                        query.CandidateTriangleBarycentrics(),
	                                        query.CandidateTriangleFrontFace()))
		{
			query.CommitNonOpaqueTriangleHit();
		}
	}

	RayTracingTraceResult result;
	result.Hit = query.CommittedStatus() == COMMITTED_TRIANGLE_HIT;
	result.FrontFace = result.Hit && query.CommittedTriangleFrontFace();
	result.RayT = result.Hit ? query.CommittedRayT() : ray.TMax;
	result.InstanceId = result.Hit ? query.CommittedInstanceID() : 0u;
	result.PrimitiveIndex = result.Hit ? query.CommittedPrimitiveIndex() : 0u;
	result.Barycentrics = result.Hit ? query.CommittedTriangleBarycentrics() : 0.0f.xx;
	return result;
}

#endif

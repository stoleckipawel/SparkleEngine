#pragma once

#include "/Engine/RayTracing/RayTracingMaterialHit.hlsli"

RayTracingTraceResult TraceRayQueryWithAlphaTest(RaytracingAccelerationStructure sceneTlas,
                                                 float3 originWorld,
                                                 float3 directionWorld,
                                                 float tMin,
                                                 float tMax,
                                                 uint rayFlags,
                                                 uint instanceMask)
{
	RayDesc ray;
	ray.Direction = normalize(directionWorld);
	ray.Origin = originWorld;
	ray.TMin = tMin;
	ray.TMax = tMax;

	RayQuery<RAY_FLAG_NONE> query;
	query.TraceRayInline(sceneTlas, rayFlags, instanceMask, ray);
	float alphaCandidateValue = 1.0f;
	float alphaCandidateCutoff = 0.5f;
	bool alphaCandidateSeen = false;
	bool alphaCandidateAccepted = false;
	bool alphaCandidateRejected = false;
	while (query.Proceed())
	{
		if (query.CandidateType() == CANDIDATE_NON_OPAQUE_TRIANGLE)
		{
			alphaCandidateSeen = true;
			const bool commitCandidate = ResolveRayTracingCandidateAlpha(query.CandidateInstanceID(),
			                                                             query.CandidatePrimitiveIndex(),
			                                                             query.CandidateTriangleBarycentrics(),
			                                                             alphaCandidateValue,
			                                                             alphaCandidateCutoff);
			if (commitCandidate)
			{
				alphaCandidateAccepted = true;
				query.CommitNonOpaqueTriangleHit();
			}
			else
			{
				alphaCandidateRejected = true;
			}
		}
	}

	RayTracingTraceResult result;
	result.Hit = query.CommittedStatus() == COMMITTED_TRIANGLE_HIT;
	result.RayT = result.Hit ? query.CommittedRayT() : ray.TMax;
	result.InstanceId = result.Hit ? query.CommittedInstanceID() : 0u;
	result.PrimitiveIndex = result.Hit ? query.CommittedPrimitiveIndex() : 0u;
	result.Barycentrics = result.Hit ? query.CommittedTriangleBarycentrics() : 0.0f.xx;
	result.AlphaCandidateSeen = alphaCandidateSeen;
	result.AlphaCandidateAccepted = alphaCandidateAccepted && result.Hit;
	result.AlphaCandidateRejected = alphaCandidateRejected;
	result.AlphaCandidateValue = alphaCandidateValue;
	result.AlphaCandidateCutoff = alphaCandidateCutoff;
	return result;
}

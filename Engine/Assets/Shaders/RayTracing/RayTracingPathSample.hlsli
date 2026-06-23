#pragma once

namespace RayTracingPathSample
{
	static const uint RejectionReasonNone = 0u;
	static const uint RejectionReasonInvalidSample = 1u;
	static const uint RejectionReasonTraceMiss = 2u;
	static const uint RejectionReasonHitSurfaceRejected = 3u;

	struct DirectionSample
	{
		float3 DirectionWorld;
		float Pdf;
		float CosineTerm;
		uint RejectionReason;
	};

	struct TraceState
	{
		bool Hit;
		float HitDistance;
		uint RejectionReason;
	};

	struct LightingResult
	{
		bool Hit;
		float3 IncidentRadiance;
		float3 Contribution;
		float HitDistance;
		uint RejectionReason;
	};
}

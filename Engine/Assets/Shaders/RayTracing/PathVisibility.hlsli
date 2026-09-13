#ifndef SPARKLE_RAY_TRACING_PATH_VISIBILITY_HLSLI
#define SPARKLE_RAY_TRACING_PATH_VISIBILITY_HLSLI

#include "/Engine/Lighting/LightSampling.hlsli"
#include "/Engine/RayTracing/RayTracingTraceQuery.hlsli"

namespace PathVisibility
{
	bool IsUnoccluded(RaytracingAccelerationStructure sceneTlas,
	                  float3 originWorld,
	                  LightSampling::DirectLightSample lightSample)
	{
		const RayTracingTraceResult trace = TraceOpaqueRayQuery(sceneTlas,
		                                                         originWorld,
		                                                         lightSample.DirectionWorld,
		                                                         0.0f,
		                                                         lightSample.VisibilityDistance,
		                                                         RAY_FLAG_SKIP_CLOSEST_HIT_SHADER,
		                                                         0xFFu);
		return !trace.Hit
		    || (trace.InstanceId == lightSample.TargetInstanceId && trace.PrimitiveIndex == lightSample.TargetPrimitiveIndex);
	}
}

#endif

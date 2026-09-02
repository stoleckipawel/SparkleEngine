#ifndef SPARKLE_RAY_TRACING_SCENE_TLAS_TRACE_HLSLI
#define SPARKLE_RAY_TRACING_SCENE_TLAS_TRACE_HLSLI

RaytracingAccelerationStructure SceneTlas;

#include "/Engine/RayTracing/RayTracingTraceQuery.hlsli"

namespace RayTracingSceneTlas
{
	RayTracingTraceResult TraceRayQueryWithAlphaTest(float3 originWorld,
	                                                 float3 directionWorld,
	                                                 float tMin,
	                                                 float tMax,
	                                                 uint rayFlags,
	                                                 uint instanceMask)
	{
		return ::TraceRayQueryWithAlphaTest(SceneTlas, originWorld, directionWorld, tMin, tMax, rayFlags, instanceMask);
	}
}

#endif

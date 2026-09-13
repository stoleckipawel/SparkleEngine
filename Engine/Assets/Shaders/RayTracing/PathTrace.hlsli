#ifndef SPARKLE_RAY_TRACING_PATH_TRACE_HLSLI
#define SPARKLE_RAY_TRACING_PATH_TRACE_HLSLI

#include "/Engine/RayTracing/PathSurface.hlsli"
#include "/Engine/RayTracing/RayEndpoints.hlsli"
#include "/Engine/RayTracing/RayTracingSceneTlasTrace.hlsli"

namespace RayTracingPathTrace
{
	RayTracingTraceResult TraceSurfaceRay(
	    RayTracingPathSurface surface,
	    float3 directionWorld,
	    out float3 rayOriginWorld)
	{
		const RayEndpoints::Ray ray = RayEndpoints::Continuation(
		    surface.PositionWorld, surface.GeometricNormalWorld, surface.PositionError, directionWorld);
		rayOriginWorld = ray.Origin;
		return RayTracingSceneTlas::TraceRayQueryWithAlphaTest(
		    ray.Origin,
		    ray.Direction,
		    ray.TMin,
		    ray.TMax,
		    RAY_FLAG_SKIP_CLOSEST_HIT_SHADER | RAY_FLAG_CULL_BACK_FACING_TRIANGLES,
		    0xFFu);
	}
}

#endif

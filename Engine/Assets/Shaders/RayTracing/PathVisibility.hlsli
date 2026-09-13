#ifndef SPARKLE_RAY_TRACING_PATH_VISIBILITY_HLSLI
#define SPARKLE_RAY_TRACING_PATH_VISIBILITY_HLSLI

#include "/Engine/Lighting/LightSampling.hlsli"
#include "/Engine/RayTracing/PathSurface.hlsli"
#include "/Engine/RayTracing/RayEndpoints.hlsli"
#include "/Engine/RayTracing/RayTracingMaterialTraceQuery.hlsli"

namespace PathVisibility
{
	bool IsUnoccluded(RaytracingAccelerationStructure sceneTlas,
	                  RayTracingPathSurface surface,
	                  LightSampling::DirectLightSample lightSample)
	{
		const RayEndpoints::Ray ray = lightSample.IsDirectional
		    ? RayEndpoints::Continuation(
		          surface.PositionWorld, surface.GeometricNormalWorld, surface.PositionError, lightSample.DirectionWorld)
		    : RayEndpoints::Connection(surface.PositionWorld,
		                               surface.GeometricNormalWorld,
		                               surface.PositionError,
		                               lightSample.SamplePositionWorld,
		                               lightSample.EmitterNormalWorld,
		                               lightSample.EmitterEndpointBaseOffset,
		                               lightSample.EmitterEndpointTraversalSensitivity);
		const RayTracingTraceResult trace = TraceRayQueryWithAlphaTest(sceneTlas,
		                                                                  ray.Origin,
		                                                                  ray.Direction,
		                                                                  ray.TMin,
		                                                                  ray.TMax,
		                                                                  RAY_FLAG_SKIP_CLOSEST_HIT_SHADER
		                                                                      | RAY_FLAG_CULL_BACK_FACING_TRIANGLES,
		                                                                  0xFFu);
		return !trace.Hit
		    || (trace.InstanceId == lightSample.TargetInstanceId && trace.PrimitiveIndex == lightSample.TargetPrimitiveIndex);
	}
}

#endif

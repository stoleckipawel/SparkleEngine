#ifndef SPARKLE_RAY_TRACING_SCENE_TRACE_PIPELINE_HLSLI
#define SPARKLE_RAY_TRACING_SCENE_TRACE_PIPELINE_HLSLI

#include "/Engine/RayTracing/RayTracingMaterialPayload.hlsli"
#include "/Engine/RayTracing/RayTracingSceneTrace.hlsli"
#include "/Engine/RayTracing/RayTracingShaderTableLayout.hlsli"

RayTracingTraceResult TraceSceneRay(RaytracingAccelerationStructure sceneTlas,
                                    float3 originWorld,
                                    float3 directionWorld,
                                    float tMin,
                                    float tMax,
                                    uint rayFlags,
                                    uint instanceMask)
{
	RayDesc ray;
	ray.Origin = originWorld;
	ray.Direction = directionWorld;
	ray.TMin = tMin;
	ray.TMax = tMax;
	RayTracingMaterialPayload payload = (RayTracingMaterialPayload)0;
	payload.RayT = tMax;
	TraceRay(sceneTlas,
	         rayFlags,
	         instanceMask,
	         RayTracingShaderTableLayout::SurfaceRayContribution,
	         RayTracingShaderTableLayout::GeometryMultiplier,
	         RayTracingShaderTableLayout::SurfaceMissIndex,
	         ray,
	         payload);
	return ResolveRayTracingMaterialPayload(payload);
}

#endif

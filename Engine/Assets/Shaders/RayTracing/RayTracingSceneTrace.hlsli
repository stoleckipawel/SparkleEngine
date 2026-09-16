#ifndef SPARKLE_RAY_TRACING_SCENE_TRACE_HLSLI
#define SPARKLE_RAY_TRACING_SCENE_TRACE_HLSLI

#include "/Engine/RayTracing/RayTracingTraceResult.hlsli"

RayTracingTraceResult TraceSceneRay(RaytracingAccelerationStructure sceneTlas,
                                    float3 originWorld,
                                    float3 directionWorld,
                                    float tMin,
                                    float tMax,
                                    uint rayFlags,
                                    uint instanceMask);

#endif

#ifndef SPARKLE_RAY_TRACING_SCENE_TLAS_TRACE_HLSLI
#define SPARKLE_RAY_TRACING_SCENE_TLAS_TRACE_HLSLI

#if defined(SPARKLE_RAY_TRACING_SCENE_TLAS_DEVICE_ADDRESS) && defined(__spirv__)
[[vk::ext_extension("SPV_KHR_ray_tracing")]]
[[vk::ext_capability(4479)]]
[[vk::ext_instruction(4447, "")]]
RaytracingAccelerationStructure SparkleConvertAddressToAccelerationStructure(uint64_t address);
#else
RaytracingAccelerationStructure SceneTlas;
#endif

#include "RayTracing/RayTracingTraceQuery.hlsli"

namespace RayTracingSceneTlas
{
	RayTracingTraceResult TraceRayQueryWithAlphaTest(
	    uint sceneTlasAddressLow,
	    uint sceneTlasAddressHigh,
	    float3 originWorld,
	    float3 directionWorld,
	    float tMin,
	    float tMax,
	    uint rayFlags,
	    uint instanceMask)
	{
#if defined(SPARKLE_RAY_TRACING_SCENE_TLAS_DEVICE_ADDRESS) && defined(__spirv__)
		const uint64_t sceneTlasAddress =
		    (uint64_t(sceneTlasAddressHigh) << 32u) | uint64_t(sceneTlasAddressLow);
		return ::TraceRayQueryWithAlphaTest(
		    SparkleConvertAddressToAccelerationStructure(sceneTlasAddress),
		    originWorld,
		    directionWorld,
		    tMin,
		    tMax,
		    rayFlags,
		    instanceMask);
#else
		return ::TraceRayQueryWithAlphaTest(SceneTlas, originWorld, directionWorld, tMin, tMax, rayFlags, instanceMask);
#endif
	}
}

#endif

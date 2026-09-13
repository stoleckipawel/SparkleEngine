#ifndef SPARKLE_RAY_TRACING_HIT_UNIFORM_DATA_HLSLI
#define SPARKLE_RAY_TRACING_HIT_UNIFORM_DATA_HLSLI

cbuffer RayTracingHitConstants
{
	uint RayTracingHitInstanceCount;
	uint RayTracingHitMaterialCount;
};

#endif

#ifndef SPARKLE_RAY_TRACING_MATERIAL_PAYLOAD_HLSLI
#define SPARKLE_RAY_TRACING_MATERIAL_PAYLOAD_HLSLI

#include "/Engine/RayTracing/RayTracingTraceResult.hlsli"

struct RayTracingMaterialPayload
{
	float RayT;
	uint InstanceId;
	uint PrimitiveIndex;
	float2 Barycentrics;
	uint Hit;
	uint FrontFace;
};

RayTracingTraceResult ResolveRayTracingMaterialPayload(RayTracingMaterialPayload payload)
{
	RayTracingTraceResult result;
	result.Hit = payload.Hit != 0u;
	result.FrontFace = payload.FrontFace != 0u;
	result.RayT = payload.RayT;
	result.InstanceId = payload.InstanceId;
	result.PrimitiveIndex = payload.PrimitiveIndex;
	result.Barycentrics = payload.Barycentrics;
	return result;
}

#endif

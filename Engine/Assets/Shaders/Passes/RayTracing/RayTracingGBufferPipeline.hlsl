#include "/Engine/Passes/RayTracing/RayTracingGBufferCommon.hlsli"

struct RayTracingGBufferPayload
{
	float RayT;
	uint InstanceId;
	uint PrimitiveIndex;
	float2 Barycentrics;
	uint Hit;
};

[shader("raygeneration")]
void RayTracingGBufferRayGeneration()
{
	const uint2 pixelCoord = DispatchRaysIndex().xy;
	const RayTracingGBuffer::PrimaryRay ray = RayTracingGBuffer::BuildPrimaryRay(pixelCoord);
	RayTracingGBufferPayload payload = (RayTracingGBufferPayload)0;
	TraceRay(SceneTlas,
	         RayTracingGBuffer::CullFlags,
	         RayTracingGBuffer::InstanceMask,
	         RayTracingShaderTableLayout::SurfaceRayContribution,
	         RayTracingShaderTableLayout::GeometryMultiplier,
	         0u,
	         ray.Description,
	         payload);

	RayTracingTraceResult trace = (RayTracingTraceResult)0;
	trace.Hit = payload.Hit != 0u;
	trace.RayT = payload.RayT;
	trace.InstanceId = payload.InstanceId;
	trace.PrimitiveIndex = payload.PrimitiveIndex;
	trace.Barycentrics = payload.Barycentrics;
	RayTracingGBuffer::StoreTraceResult(pixelCoord, trace, ray);
}

[shader("anyhit")]
void RayTracingGBufferAnyHit(inout RayTracingGBufferPayload payload,
	BuiltInTriangleIntersectionAttributes attributes)
{
	float sampledAlpha = 1.0f;
	float alphaCutoff = 0.5f;
	if (!ResolveRayTracingCandidateAlpha(InstanceID(), PrimitiveIndex(), attributes.barycentrics, sampledAlpha, alphaCutoff))
	{
		IgnoreHit();
	}
}

[shader("miss")]
void RayTracingGBufferMiss(inout RayTracingGBufferPayload payload)
{
	payload.Hit = 0u;
}

[shader("closesthit")]
void RayTracingGBufferClosestHit(inout RayTracingGBufferPayload payload,
	BuiltInTriangleIntersectionAttributes attributes)
{
	payload.RayT = RayTCurrent();
	payload.InstanceId = InstanceID();
	payload.PrimitiveIndex = PrimitiveIndex();
	payload.Barycentrics = attributes.barycentrics;
	payload.Hit = 1u;
}

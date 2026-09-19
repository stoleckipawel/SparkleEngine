#include "/Engine/RayTracing/RayTracingMaterialAlpha.hlsli"
#include "/Engine/RayTracing/RayTracingMaterialPayload.hlsli"

[shader("miss")]
void RayTracingMaterialMiss(inout RayTracingMaterialPayload payload)
{
	payload.Hit = 0u;
}

[shader("closesthit")]
void RayTracingMaterialClosestHit(inout RayTracingMaterialPayload payload,
	BuiltInTriangleIntersectionAttributes attributes)
{
	payload.RayT = RayTCurrent();
	payload.InstanceId = InstanceID();
	payload.PrimitiveIndex = PrimitiveIndex();
	payload.Barycentrics = attributes.barycentrics;
	payload.Hit = 1u;
	payload.FrontFace = HitKind() == HIT_KIND_TRIANGLE_FRONT_FACE ? 1u : 0u;
}

[shader("anyhit")]
void RayTracingMaterialAnyHit(inout RayTracingMaterialPayload payload, BuiltInTriangleIntersectionAttributes attributes)
{
	(void)payload;
	if (!ResolveRayTracingCandidateAlpha(InstanceID(),
	                                     PrimitiveIndex(),
	                                     attributes.barycentrics,
	                                     HitKind() == HIT_KIND_TRIANGLE_FRONT_FACE))
	{
		IgnoreHit();
	}
}

#include "/Engine/Passes/Lighting/Shadows/DirectShadowSignalCommon.hlsli"
#include "/Engine/RayTracing/RayTracingMaterialHit.hlsli"
#include "/Engine/RayTracing/RayTracingShaderTableLayout.hlsli"

RaytracingAccelerationStructure SceneTlas;

struct DirectShadowSignalPayload
{
	float HitDistance;
	uint Occluded;
};

[shader("raygeneration")]
void DirectShadowSignalRayGeneration()
{
	const uint2 pixelCoord = DispatchRaysIndex().xy;
	RayTracedShadowRequest request = (RayTracedShadowRequest)0;
	ShadowVisibilitySample signal = RayTracedShadowSignals::BuildUnshadowedSignal(0.0f);
	bool validPixel = false;
	if (PrepareDirectShadowSignal(pixelCoord, validPixel, request, signal))
	{
		DirectShadowSignalPayload payload;
		payload.HitDistance = request.MaxDistance;
		payload.Occluded = 0u;
		RayDesc ray;
		ray.Origin = request.OriginWorld;
		ray.Direction = normalize(request.DirectionWorld);
		ray.TMin = RayTracedShadows::MinimumShadowTMin;
		ray.TMax = request.MaxDistance;
		TraceRay(SceneTlas,
		         RayTracedShadows::ShadowRayFlags,
		         RayTracedShadows::ShadowInstanceMask,
		         RayTracingShaderTableLayout::ShadowVisibilityRayContribution,
		         RayTracingShaderTableLayout::GeometryMultiplier,
		         0u,
		         ray,
		         payload);
		signal = RayTracedShadows::ResolveTrace(payload.Occluded != 0u, payload.HitDistance, request.MaxDistance);
	}
	if (validPixel)
	{
		StoreDirectShadowSignal(pixelCoord, signal);
	}
}

[shader("miss")]
void DirectShadowSignalMiss(inout DirectShadowSignalPayload payload)
{
	payload.Occluded = 0u;
}

[shader("closesthit")]
void DirectShadowSignalClosestHit(inout DirectShadowSignalPayload payload,
	BuiltInTriangleIntersectionAttributes attributes)
{
	(void)attributes;
	payload.HitDistance = RayTCurrent();
	payload.Occluded = 1u;
}

[shader("anyhit")]
void DirectShadowSignalAnyHit(inout DirectShadowSignalPayload payload,
	BuiltInTriangleIntersectionAttributes attributes)
{
	(void)payload;
	float sampledAlpha = 1.0f;
	float alphaCutoff = 0.5f;
	if (!ResolveRayTracingCandidateAlpha(InstanceID(), PrimitiveIndex(), attributes.barycentrics, sampledAlpha, alphaCutoff))
	{
		IgnoreHit();
	}
}

#include "/Engine/Passes/RayTracing/RayTracingGBufferCommon.hlsli"
#include "/Engine/RayTracing/RayTracingTraceQuery.hlsli"

[numthreads(8, 8, 1)] void RayTracingGBufferInline(uint3 dispatchThreadId : SV_DispatchThreadID)
{
	uint width = 0;
	uint height = 0;
	GBufferBaseColor.GetDimensions(width, height);
	if (dispatchThreadId.x >= width || dispatchThreadId.y >= height)
	{
		return;
	}

	const uint2 pixelCoord = dispatchThreadId.xy;
	const RayTracingGBuffer::PrimaryRay ray = RayTracingGBuffer::BuildPrimaryRay(pixelCoord);
	const RayTracingTraceResult trace = TraceRayQueryWithAlphaTest(
	    SceneTlas,
	    ray.OriginWorld,
	    ray.DirectionWorld,
	    ray.Description.TMin,
	    ray.Description.TMax,
	    RayTracingGBuffer::CullFlags | RAY_FLAG_SKIP_CLOSEST_HIT_SHADER,
	    RayTracingGBuffer::InstanceMask);
	RayTracingGBuffer::StoreTraceResult(pixelCoord, trace, ray);
}

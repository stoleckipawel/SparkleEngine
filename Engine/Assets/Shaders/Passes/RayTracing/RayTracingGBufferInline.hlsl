#include "/Engine/Passes/RayTracing/RayTracingGBufferCommon.hlsli"
#include "/Engine/RayTracing/RayTracingSceneTraceInline.hlsli"

[numthreads(8, 8, 1)]
void RayTracingGBufferInline(uint3 dispatchThreadId : SV_DispatchThreadID)
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
	const RayTracingTraceResult trace = TraceSceneRay(SceneTlas,
	                                                              ray.OriginWorld,
	                                                              ray.DirectionWorld,
	                                                              ray.Description.TMin,
	                                                              ray.Description.TMax,
	                                                              RayTracingGBuffer::CullFlags,
	                                                              RayTracingGBuffer::InstanceMask);
	RayTracingGBuffer::StoreTraceResult(pixelCoord, trace, ray);
}

#include "/Engine/Passes/RayTracing/RayTracingGBufferCommon.hlsli"
#include "/Engine/RayTracing/RayTracingSceneTracePipeline.hlsli"

[shader("raygeneration")]
void RayTracingGBufferRayGeneration()
{
	const uint2 pixelCoord = DispatchRaysIndex().xy;
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

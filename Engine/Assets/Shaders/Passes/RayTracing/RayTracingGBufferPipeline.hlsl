#include "/Engine/RayTracing/RayTracingSceneTracePipeline.hlsli"
#include "/Engine/Passes/RayTracing/RayTracingGBufferCommon.hlsli"

[shader("raygeneration")] void RayTracingGBufferRayGeneration()
{
	RayTracingGBuffer::TraceAndStore(DispatchRaysIndex().xy);
}

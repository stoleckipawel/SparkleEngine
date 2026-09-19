#include "/Engine/RayTracing/RayTracingSceneTraceInline.hlsli"
#include "/Engine/Passes/GBuffer/RayTracing/RayTracingGBufferCommon.hlsli"

[numthreads(8, 8, 1)] void RayTracingGBufferInline(uint3 dispatchThreadId : SV_DispatchThreadID)
{
	uint width = 0;
	uint height = 0;
	GBufferBaseColor.GetDimensions(width, height);
	if (dispatchThreadId.x >= width || dispatchThreadId.y >= height)
	{
		return;
	}

	RayTracingGBuffer::TraceAndStore(dispatchThreadId.xy);
}

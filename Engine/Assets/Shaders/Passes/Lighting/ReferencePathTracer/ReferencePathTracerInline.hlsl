#include "/Engine/RayTracing/RayTracingSceneTraceInline.hlsli"
#include "/Engine/Passes/Lighting/ReferencePathTracer/ReferencePathTracerKernel.hlsli"

[numthreads(8, 8, 1)]
void ReferencePathTracerInline(uint3 dispatchThreadId : SV_DispatchThreadID)
{
	ReferencePathTracer::TraceAndAccumulate(dispatchThreadId.xy);
}

#include "/Engine/RayTracing/RayTracingSceneTracePipeline.hlsli"
#include "/Engine/Passes/Lighting/ReferencePathTracer/ReferencePathTracerKernel.hlsli"

[shader("raygeneration")]
void ReferencePathTracerRayGeneration()
{
	ReferencePathTracer::TraceAndAccumulate(DispatchRaysIndex().xy);
}

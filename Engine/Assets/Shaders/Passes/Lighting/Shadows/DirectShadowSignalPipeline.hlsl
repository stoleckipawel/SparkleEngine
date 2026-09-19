#include "/Engine/RayTracing/RayTracingSceneTracePipeline.hlsli"
#include "/Engine/Passes/Lighting/Shadows/DirectShadowSignalCommon.hlsli"

[shader("raygeneration")]
void DirectShadowSignalRayGeneration()
{
	TraceAndStoreDirectShadowSignal(DispatchRaysIndex().xy);
}

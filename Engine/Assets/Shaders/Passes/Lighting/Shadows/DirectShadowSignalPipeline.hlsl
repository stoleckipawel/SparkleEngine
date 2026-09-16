#include "/Engine/Passes/Lighting/Shadows/DirectShadowSignalCommon.hlsli"
#include "/Engine/RayTracing/RayTracingSceneTracePipeline.hlsli"

RaytracingAccelerationStructure SceneTlas;

[shader("raygeneration")]
void DirectShadowSignalRayGeneration()
{
	const uint2 pixelCoord = DispatchRaysIndex().xy;
	RayTracedShadowRequest request = (RayTracedShadowRequest)0;
	ShadowVisibilitySample signal = RayTracedShadowSignals::BuildUnshadowedSignal(0.0f);
	bool validPixel = false;
	if (PrepareDirectShadowSignal(pixelCoord, validPixel, request, signal))
	{
		const RayTracingTraceResult trace = TraceSceneRay(SceneTlas,
		                                                   request.OriginWorld,
		                                                   normalize(request.DirectionWorld),
		                                                   RayTracedShadows::MinimumShadowTMin,
		                                                   request.MaxDistance,
		                                                   RayTracedShadows::ShadowRayFlags,
		                                                   RayTracedShadows::ShadowInstanceMask);
		signal = RayTracedShadows::ResolveTrace(trace.Hit, trace.RayT, request.MaxDistance);
	}
	if (validPixel)
	{
		StoreDirectShadowSignal(pixelCoord, signal);
	}
}

#include "/Engine/Passes/Lighting/Shadows/DirectShadowSignalCommon.hlsli"
#include "/Engine/RayTracing/Shadows/RayTracedShadowTrace.hlsli"

[numthreads(8, 8, 1)] void main(uint3 dispatchThreadId : SV_DispatchThreadID)
{
	RayTracedShadowRequest request = (RayTracedShadowRequest)0;
	ShadowVisibilitySample signal = RayTracedShadowSignals::BuildUnshadowedSignal(0.0f);
	bool validPixel = false;
	if (PrepareDirectShadowSignal(dispatchThreadId.xy, validPixel, request, signal))
	{
		signal = RayTracedShadows::TraceShadowRay(request);
	}
	if (validPixel)
	{
		StoreDirectShadowSignal(dispatchThreadId.xy, signal);
	}
}

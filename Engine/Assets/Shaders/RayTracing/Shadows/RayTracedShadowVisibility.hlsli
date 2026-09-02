#ifndef SPARKLE_RAY_TRACED_SHADOW_VISIBILITY_HLSLI
#define SPARKLE_RAY_TRACED_SHADOW_VISIBILITY_HLSLI

#include "/Engine/Lighting/LightSampling.hlsli"
#include "/Engine/RayTracing/Shadows/RayTracedShadowSignals.hlsli"

#include "/Engine/RayTracing/Shadows/RayTracedShadowTrace.hlsli"

namespace RayTracedShadowVisibility
{
	ShadowVisibilitySample TraceDirectLightSample(float3 positionWorld,
	                                              float3 normalWorld,
	                                              LightSampling::DirectLightSample lightSample,
	                                              bool castsShadow)
	{
		RayTracedShadowRequest request = (RayTracedShadowRequest)0;
		ShadowVisibilitySample signal = RayTracedShadowSignals::BuildUnshadowedSignal(0.0f);
		if (RayTracedShadows::BuildDirectLightRequest(positionWorld, normalWorld, lightSample, castsShadow, request, signal))
		{
			return RayTracedShadows::TraceShadowRay(request);
		}
		return signal;
	}
}

#endif

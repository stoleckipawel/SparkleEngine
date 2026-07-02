#ifndef SPARKLE_RAY_TRACED_SHADOW_VISIBILITY_HLSLI
#define SPARKLE_RAY_TRACED_SHADOW_VISIBILITY_HLSLI

#include "Lighting/LightSampling.hlsli"
#include "RayTracing/Shadows/RayTracedShadowSignals.hlsli"

#if !defined(SPARKLE_RAY_TRACED_SHADOWS_DISABLED)
#include "RayTracing/Shadows/RayTracedShadowTrace.hlsli"
#endif

namespace RayTracedShadowVisibility
{
	ShadowVisibilitySignal TraceDirectLightSample(
	    float3 positionWorld,
	    float3 normalWorld,
	    LightSampling::DirectLightSample lightSample,
	    bool castsShadow)
	{
		if (!lightSample.Valid)
		{
			return RayTracedShadowSignals::BuildUnshadowedSignal(0.0f);
		}

#if defined(SPARKLE_RAY_TRACED_SHADOWS_DISABLED)
		return RayTracedShadowSignals::BuildUnshadowedSignal(lightSample.VisibilityDistance);
#else
		return RayTracedShadows::TraceDirectLightSample(
		    positionWorld,
		    normalWorld,
		    lightSample.DirectionWorld,
		    lightSample.VisibilityDistance,
		    lightSample.IsDirectional,
		    castsShadow);
#endif
	}
}

#endif

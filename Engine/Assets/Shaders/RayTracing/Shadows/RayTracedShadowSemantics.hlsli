#ifndef SPARKLE_RAY_TRACED_SHADOW_SEMANTICS_HLSLI
#define SPARKLE_RAY_TRACED_SHADOW_SEMANTICS_HLSLI

#include "/Engine/Lighting/LightSampling.hlsli"
#include "/Engine/RayTracing/Shadows/RayTracedShadowSignals.hlsli"

cbuffer RayTracedShadowConstants
{
	uint RayTracedDirectionalShadowsEnabled;
	uint RayTracedLocalLightShadowsEnabled;
	uint RayTracingHitInstanceCount;
	uint RayTracingHitMaterialCount;
	float RayTracedShadowNormalBias;
	float RayTracedShadowMaxDistance;
	float RayTracedShadowPadding2;
	float RayTracedShadowPadding3;
};

struct RayTracedShadowRequest
{
	float3 OriginWorld;
	float3 DirectionWorld;
	float MaxDistance;
};

namespace RayTracedShadows
{
	static const uint ShadowInstanceMask = 0xFFu;
	static const uint ShadowRayFlags = RAY_FLAG_SKIP_PROCEDURAL_PRIMITIVES;
	static const float MinimumShadowTMin = 0.001f;

	bool BuildDirectLightRequest(
	    float3 positionWorld,
	    float3 normalWorld,
	    LightSampling::DirectLightSample lightSample,
	    bool castsShadow,
	    out RayTracedShadowRequest request,
	    out ShadowVisibilitySample immediateSignal)
	{
		request = (RayTracedShadowRequest)0;
		immediateSignal = RayTracedShadowSignals::BuildUnshadowedSignal(0.0f);
		if (!lightSample.Valid)
		{
			return false;
		}

		const bool supported = lightSample.IsDirectional ? RayTracedDirectionalShadowsEnabled != 0u
		                                                   : RayTracedLocalLightShadowsEnabled != 0u;
		const float maxDistance = lightSample.IsDirectional ? RayTracedShadowMaxDistance : lightSample.VisibilityDistance;
		immediateSignal = RayTracedShadowSignals::BuildUnshadowedSignal(maxDistance);
		if (!castsShadow || !supported || (!lightSample.IsDirectional && lightSample.VisibilityDistance <= MinimumShadowTMin))
		{
			return false;
		}

		request.OriginWorld = positionWorld + normalize(normalWorld) * RayTracedShadowNormalBias;
		request.DirectionWorld = lightSample.DirectionWorld;
		request.MaxDistance = lightSample.IsDirectional
		    ? RayTracedShadowMaxDistance
		    : max(lightSample.VisibilityDistance - MinimumShadowTMin, MinimumShadowTMin);
		return true;
	}

	ShadowVisibilitySample ResolveTrace(bool occluded, float hitDistance, float maxDistance)
	{
		return occluded ? RayTracedShadowSignals::BuildOccludedSignal(hitDistance, maxDistance)
		                : RayTracedShadowSignals::BuildUnshadowedSignal(maxDistance);
	}
}

#endif

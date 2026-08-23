#ifndef SPARKLE_RAY_TRACED_SHADOW_TRACE_HLSLI
#define SPARKLE_RAY_TRACED_SHADOW_TRACE_HLSLI

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

#include "/Engine/RayTracing/RayTracingSceneTlasTrace.hlsli"

namespace RayTracedShadows
{
	static const uint ShadowInstanceMask = 0xFFu;
	static const uint ShadowRayFlags = RAY_FLAG_SKIP_PROCEDURAL_PRIMITIVES;
	static const float MinimumShadowTMin = 0.001f;

	bool SupportsDirectionalShadows()
	{
		return RayTracedDirectionalShadowsEnabled != 0u;
	}

	bool SupportsLocalLightShadows()
	{
		return RayTracedLocalLightShadowsEnabled != 0u;
	}

	float3 BuildRayOrigin(float3 positionWorld, float3 normalWorld)
	{
		return positionWorld + normalize(normalWorld) * RayTracedShadowNormalBias;
	}

	ShadowVisibilitySignal TraceShadowRay(float3 originWorld, float3 directionWorld, float maxDistance)
	{
		const float clampedMaxDistance = max(maxDistance, MinimumShadowTMin);
		const RayTracingTraceResult trace = RayTracingSceneTlas::TraceRayQueryWithAlphaTest(originWorld,
		                                                                                    directionWorld,
		                                                                                    MinimumShadowTMin,
		                                                                                    clampedMaxDistance,
		                                                                                    ShadowRayFlags,
		                                                                                    ShadowInstanceMask);

		if (trace.Hit)
		{
			return RayTracedShadowSignals::BuildOccludedSignal(trace.RayT, clampedMaxDistance);
		}

		return RayTracedShadowSignals::BuildUnshadowedSignal(clampedMaxDistance);
	}

	ShadowVisibilitySignal TraceDirectLightSample(float3 positionWorld,
	                                              float3 normalWorld,
	                                              float3 lightDirectionWorld,
	                                              float lightDistance,
	                                              bool directionalLight,
	                                              bool castsShadow)
	{
		const bool supported = directionalLight ? SupportsDirectionalShadows() : SupportsLocalLightShadows();
		const float maxDistance = directionalLight ? RayTracedShadowMaxDistance : lightDistance;
		if (!castsShadow || !supported)
		{
			return RayTracedShadowSignals::BuildUnshadowedSignal(maxDistance);
		}

		if (!directionalLight && lightDistance <= MinimumShadowTMin)
		{
			return RayTracedShadowSignals::BuildUnshadowedSignal(lightDistance);
		}

		const float3 originWorld = BuildRayOrigin(positionWorld, normalWorld);
		const float traceDistance =
		    directionalLight ? RayTracedShadowMaxDistance : max(lightDistance - MinimumShadowTMin, MinimumShadowTMin);
		return TraceShadowRay(originWorld, lightDirectionWorld, traceDistance);
	}
}

#endif

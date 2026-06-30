#ifndef SPARKLE_RAY_TRACED_SHADOW_TRACE_HLSLI
#define SPARKLE_RAY_TRACED_SHADOW_TRACE_HLSLI

#include "RayTracing/Shadows/RayTracedShadowSignals.hlsli"

cbuffer RayTracedShadowUniformData
{
	uint RayTracedDirectionalShadowsEnabled;
	uint RayTracedLocalLightShadowsEnabled;
	uint RayTracedShadowDiagnosticsEnabled;
	uint RayTracedShadowPadding0;
	float RayTracedShadowNormalBias;
	float RayTracedShadowMaxDistance;
	float RayTracedShadowPadding1;
	float RayTracedShadowPadding2;
	uint RayTracedShadowSceneTlasGpuAddressLow;
	uint RayTracedShadowSceneTlasGpuAddressHigh;
	uint RayTracingHitDataAvailable;
	uint RayTracingHitInstanceCount;
	uint RayTracingHitMaterialCount;
	uint RayTracedShadowPadding3;
	uint RayTracedShadowPadding4;
	uint RayTracedShadowPadding5;
};

#include "RayTracing/RayTracingSceneTlasTrace.hlsli"

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
		const RayTracingTraceResult trace = RayTracingSceneTlas::TraceRayQueryWithAlphaTest(
		    RayTracedShadowSceneTlasGpuAddressLow,
		    RayTracedShadowSceneTlasGpuAddressHigh,
		    originWorld,
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

	ShadowVisibilitySignal TraceDirectLightSample(
	    float3 positionWorld,
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
		const float traceDistance = directionalLight ? RayTracedShadowMaxDistance : max(lightDistance - MinimumShadowTMin, MinimumShadowTMin);
		return TraceShadowRay(originWorld, lightDirectionWorld, traceDistance);
	}
}

#endif

#ifndef SPARKLE_RAY_TRACED_SHADOW_TRACE_HLSLI
#define SPARKLE_RAY_TRACED_SHADOW_TRACE_HLSLI

#include "RayTracing/Shadows/RayTracedShadowDenoiserInputs.hlsli"
#include "RayTracing/Shadows/RayTracedShadowSignals.hlsli"

#if !defined(SPARKLE_DIRECT_LIGHTING_VULKAN_ADDRESS) || !defined(__spirv__)
RaytracingAccelerationStructure SceneTlas;
#endif

#if defined(SPARKLE_DIRECT_LIGHTING_VULKAN_ADDRESS) && defined(__spirv__)
[[vk::ext_extension("SPV_KHR_ray_tracing")]]
[[vk::ext_capability(4479)]]
[[vk::ext_instruction(4447, "")]]
RaytracingAccelerationStructure SparkleConvertAddressToAccelerationStructure(uint64_t address);
#endif

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
	uint RayTracedShadowTlasAccessMode;
	uint RayTracedShadowPadding3;
};

namespace RayTracedShadows
{
	static const uint TlasAccessModeDescriptor = 0u;
	static const uint TlasAccessModeShaderDeviceAddress = 1u;
	static const uint ShadowInstanceMask = 0xFFu;
	static const uint ShadowRayFlags = RAY_FLAG_ACCEPT_FIRST_HIT_AND_END_SEARCH | RAY_FLAG_SKIP_PROCEDURAL_PRIMITIVES;
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
		RayDesc shadowRay;
		shadowRay.Origin = originWorld;
		shadowRay.Direction = normalize(directionWorld);
		shadowRay.TMin = MinimumShadowTMin;
		shadowRay.TMax = clampedMaxDistance;

		RayQuery<ShadowRayFlags> query;
#if defined(SPARKLE_DIRECT_LIGHTING_VULKAN_ADDRESS) && defined(__spirv__)
		const uint64_t sceneTlasAddress =
		    (uint64_t(RayTracedShadowSceneTlasGpuAddressHigh) << 32u) | uint64_t(RayTracedShadowSceneTlasGpuAddressLow);
		query.TraceRayInline(
		    SparkleConvertAddressToAccelerationStructure(sceneTlasAddress),
		    ShadowRayFlags,
		    ShadowInstanceMask,
		    shadowRay);
#else
		query.TraceRayInline(SceneTlas, ShadowRayFlags, ShadowInstanceMask, shadowRay);
#endif
		while (query.Proceed())
		{
		}

		if (query.CommittedStatus() == COMMITTED_TRIANGLE_HIT)
		{
			return RayTracedShadowSignals::BuildOccludedSignal(query.CommittedRayT(), clampedMaxDistance);
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

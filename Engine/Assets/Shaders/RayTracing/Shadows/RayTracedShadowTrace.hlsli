#ifndef SPARKLE_RAY_TRACED_SHADOW_TRACE_HLSLI
#define SPARKLE_RAY_TRACED_SHADOW_TRACE_HLSLI

#include "RayTracing/Shadows/RayTracedShadowDenoiserInputs.hlsli"
#include "RayTracing/Shadows/RayTracedShadowSampling.hlsli"
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
	uint RayTracedShadowQualityMode;
	float RayTracedShadowNormalBias;
	float RayTracedShadowMaxDistance;
	float RayTracedShadowPadding0;
	float RayTracedShadowPadding1;
	uint RayTracedShadowSceneTlasGpuAddressLow;
	uint RayTracedShadowSceneTlasGpuAddressHigh;
	uint RayTracedShadowTlasAccessMode;
	uint RayTracedShadowPadding2;
};

namespace RayTracedShadows
{
	static const uint TlasAccessModeDescriptor = 0u;
	static const uint TlasAccessModeShaderDeviceAddress = 1u;
	static const uint ShadowInstanceMask = 0xFFu;
	static const uint ShadowRayFlags = RAY_FLAG_ACCEPT_FIRST_HIT_AND_END_SEARCH | RAY_FLAG_SKIP_PROCEDURAL_PRIMITIVES;
	static const float MinimumShadowTMin = 0.001f;
	static const uint ShadowQualityModeHard = 0u;
	static const uint ShadowQualityModeSoftAreaLights = 1u;

	bool SupportsDirectionalShadows()
	{
		return RayTracedDirectionalShadowsEnabled != 0u;
	}

	bool SupportsLocalLightShadows()
	{
		return RayTracedLocalLightShadowsEnabled != 0u;
	}

	bool UsesHardShadowVisibility()
	{
		return RayTracedShadowQualityMode == ShadowQualityModeHard;
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

	ShadowVisibilitySignal TraceDirectionalShadowSignal(
	    float3 positionWorld,
	    float3 normalWorld,
	    float3 lightDirectionWorld,
	    float angularDiameterRadians,
	    uint2 pixelCoord,
	    uint lightIndex,
	    bool castsShadow)
	{
		if (!castsShadow || !SupportsDirectionalShadows())
		{
			return RayTracedShadowSignals::BuildUnshadowedSignal(RayTracedShadowMaxDistance);
		}

		const float3 originWorld = BuildRayOrigin(positionWorld, normalWorld);
		if (UsesHardShadowVisibility())
		{
			return TraceShadowRay(originWorld, lightDirectionWorld, RayTracedShadowMaxDistance);
		}

		const float2 sample = RayTracedShadowSampling::BuildAnimatedSample(pixelCoord, lightIndex, 0u);
		const float coneHalfAngle = max(angularDiameterRadians, 0.0f) * 0.5f;
		const float3 sampledDirection = RayTracedShadowSampling::SampleConeDirection(lightDirectionWorld, coneHalfAngle, sample);
		return TraceShadowRay(originWorld, sampledDirection, RayTracedShadowMaxDistance);
	}

	ShadowVisibilitySignal TracePointShadowSignal(
	    float3 positionWorld,
	    float3 normalWorld,
	    float3 lightPositionWorld,
	    float lightRange,
	    float sourceRadius,
	    uint2 pixelCoord,
	    uint lightIndex,
	    bool castsShadow)
	{
		if (!castsShadow || !SupportsLocalLightShadows())
		{
			return RayTracedShadowSignals::BuildUnshadowedSignal(RayTracedShadowMaxDistance);
		}

		float3 sampledLightPosition = lightPositionWorld;
		if (!UsesHardShadowVisibility())
		{
			const float2 sample = RayTracedShadowSampling::BuildAnimatedSample(pixelCoord, lightIndex, 1u);
			sampledLightPosition = RayTracedShadowSampling::SampleSpherePoint(
			    lightPositionWorld,
			    max(sourceRadius, 0.0f),
			    positionWorld - lightPositionWorld,
			    sample);
		}
		const float3 surfaceToLight = sampledLightPosition - positionWorld;
		const float distanceToLight = length(surfaceToLight);
		if (distanceToLight <= MinimumShadowTMin)
		{
			return RayTracedShadowSignals::BuildUnshadowedSignal(MinimumShadowTMin);
		}

		const float maxDistance = lightRange > 0.0f ? min(distanceToLight, lightRange) : distanceToLight;
		if (maxDistance <= MinimumShadowTMin)
		{
			return RayTracedShadowSignals::BuildUnshadowedSignal(maxDistance);
		}

		const float3 originWorld = BuildRayOrigin(positionWorld, normalWorld);
		return TraceShadowRay(originWorld, surfaceToLight, maxDistance - MinimumShadowTMin);
	}

	ShadowVisibilitySignal TraceSpotShadowSignal(
	    float3 positionWorld,
	    float3 normalWorld,
	    float3 lightPositionWorld,
	    float3 spotDirectionWorld,
	    float lightRange,
	    float sourceRadius,
	    float outerConeCosine,
	    uint2 pixelCoord,
	    uint lightIndex,
	    bool castsShadow)
	{
		if (!castsShadow || !SupportsLocalLightShadows())
		{
			return RayTracedShadowSignals::BuildUnshadowedSignal(RayTracedShadowMaxDistance);
		}

		float3 sampledLightPosition = lightPositionWorld;
		if (!UsesHardShadowVisibility())
		{
			const float2 sample = RayTracedShadowSampling::BuildAnimatedSample(pixelCoord, lightIndex, 2u);
			sampledLightPosition = RayTracedShadowSampling::SampleDiskPoint(
			    lightPositionWorld,
			    spotDirectionWorld,
			    max(sourceRadius, 0.0f),
			    sample);
		}
		const float3 surfaceToLight = sampledLightPosition - positionWorld;
		const float distanceToLight = length(surfaceToLight);
		if (distanceToLight <= MinimumShadowTMin)
		{
			return RayTracedShadowSignals::BuildUnshadowedSignal(MinimumShadowTMin);
		}

		const float3 lightDirection = surfaceToLight / max(distanceToLight, 0.0001f);
		const float coneVisibility = dot(-lightDirection, normalize(spotDirectionWorld));
		if (coneVisibility < outerConeCosine)
		{
			return RayTracedShadowSignals::BuildUnshadowedSignal(distanceToLight);
		}

		const float maxDistance = lightRange > 0.0f ? min(distanceToLight, lightRange) : distanceToLight;
		if (maxDistance <= MinimumShadowTMin)
		{
			return RayTracedShadowSignals::BuildUnshadowedSignal(maxDistance);
		}

		const float3 originWorld = BuildRayOrigin(positionWorld, normalWorld);
		return TraceShadowRay(originWorld, surfaceToLight, maxDistance - MinimumShadowTMin);
	}
}

#endif

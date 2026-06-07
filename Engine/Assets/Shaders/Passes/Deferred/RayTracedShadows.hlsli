#ifndef SPARKLE_RAY_TRACED_SHADOWS_HLSLI
#define SPARKLE_RAY_TRACED_SHADOWS_HLSLI

RaytracingAccelerationStructure SceneTlas;

cbuffer RayTracedShadowUniformData
{
	uint RayTracedDirectionalShadowsEnabled;
	uint RayTracedLocalLightShadowsEnabled;
	uint RayTracedShadowDiagnosticsEnabled;
	uint RayTracedShadowRaysPerPixel;
	float RayTracedShadowNormalBias;
	float RayTracedShadowMaxDistance;
	float RayTracedShadowPadding0;
	float RayTracedShadowPadding1;
};

namespace RayTracedShadows
{
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

	float TraceShadowRay(float3 originWorld, float3 directionWorld, float maxDistance)
	{
		RayDesc shadowRay;
		shadowRay.Origin = originWorld;
		shadowRay.Direction = normalize(directionWorld);
		shadowRay.TMin = MinimumShadowTMin;
		shadowRay.TMax = max(maxDistance, MinimumShadowTMin);

		RayQuery<ShadowRayFlags> query;
		query.TraceRayInline(SceneTlas, ShadowRayFlags, ShadowInstanceMask, shadowRay);
		while (query.Proceed())
		{
		}

		return query.CommittedStatus() == COMMITTED_TRIANGLE_HIT ? 0.0f : 1.0f;
	}

	float TraceDirectionalShadow(float3 positionWorld, float3 normalWorld, float3 lightDirectionWorld, bool castsShadow)
	{
		if (!castsShadow || !SupportsDirectionalShadows())
		{
			return 1.0f;
		}

		const float3 originWorld = positionWorld + normalize(normalWorld) * RayTracedShadowNormalBias;
		return TraceShadowRay(originWorld, lightDirectionWorld, RayTracedShadowMaxDistance);
	}

	float TracePointShadow(float3 positionWorld, float3 normalWorld, float3 lightPositionWorld, float lightRange, bool castsShadow)
	{
		if (!castsShadow || !SupportsLocalLightShadows())
		{
			return 1.0f;
		}

		const float3 surfaceToLight = lightPositionWorld - positionWorld;
		const float distanceToLight = length(surfaceToLight);
		if (distanceToLight <= MinimumShadowTMin)
		{
			return 1.0f;
		}

		const float maxDistance = lightRange > 0.0f ? min(distanceToLight, lightRange) : distanceToLight;
		if (maxDistance <= MinimumShadowTMin)
		{
			return 1.0f;
		}

		const float3 originWorld = positionWorld + normalize(normalWorld) * RayTracedShadowNormalBias;
		return TraceShadowRay(originWorld, surfaceToLight, maxDistance - MinimumShadowTMin);
	}

	float TraceSpotShadow(
	    float3 positionWorld,
	    float3 normalWorld,
	    float3 lightPositionWorld,
	    float3 spotDirectionWorld,
	    float lightRange,
	    float outerConeCosine,
	    bool castsShadow)
	{
		if (!castsShadow || !SupportsLocalLightShadows())
		{
			return 1.0f;
		}

		const float3 surfaceToLight = lightPositionWorld - positionWorld;
		const float distanceToLight = length(surfaceToLight);
		if (distanceToLight <= MinimumShadowTMin)
		{
			return 1.0f;
		}

		const float3 lightDirection = surfaceToLight / max(distanceToLight, 0.0001f);
		const float coneVisibility = dot(-lightDirection, normalize(spotDirectionWorld));
		if (coneVisibility < outerConeCosine)
		{
			return 1.0f;
		}

		const float maxDistance = lightRange > 0.0f ? min(distanceToLight, lightRange) : distanceToLight;
		if (maxDistance <= MinimumShadowTMin)
		{
			return 1.0f;
		}

		const float3 originWorld = positionWorld + normalize(normalWorld) * RayTracedShadowNormalBias;
		return TraceShadowRay(originWorld, surfaceToLight, maxDistance - MinimumShadowTMin);
	}
}

#endif

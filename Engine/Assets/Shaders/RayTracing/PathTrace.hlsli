#ifndef SPARKLE_RAY_TRACING_PATH_TRACE_HLSLI
#define SPARKLE_RAY_TRACING_PATH_TRACE_HLSLI

#include "/Engine/RayTracing/PathSurface.hlsli"
#include "/Engine/RayTracing/RayTracingSceneTlasTrace.hlsli"

namespace RayTracingPathTrace
{
	struct TraceSettings
	{
		float NormalBias;
		float MaxDistance;
		float MinT;
		uint RayFlags;
		uint InstanceMask;
	};

	TraceSettings BuildSurfaceTraceSettings(float normalBias, float maxDistance)
	{
		TraceSettings settings;
		settings.NormalBias = normalBias;
		settings.MaxDistance = maxDistance;
		settings.MinT = 0.001f;
		settings.RayFlags = RAY_FLAG_SKIP_CLOSEST_HIT_SHADER | RAY_FLAG_CULL_BACK_FACING_TRIANGLES;
		settings.InstanceMask = 0xFFu;
		return settings;
	}

	float3 ComputeRayOrigin(RayTracingPathSurface surface, float3 rayDirectionWorld, TraceSettings settings)
	{
		const float bias = max(settings.NormalBias, 0.0f);
		const float NoR = abs(dot(surface.NormalWorld, rayDirectionWorld));
		const float grazingScale = rcp(max(NoR, 0.25f));
		return surface.PositionWorld + surface.NormalWorld * bias * grazingScale + rayDirectionWorld * settings.MinT;
	}

	RayTracingTraceResult TraceSceneRay(float3 originWorld, float3 directionWorld, TraceSettings settings)
	{
		return RayTracingSceneTlas::TraceRayQueryWithAlphaTest(
		    originWorld,
		    directionWorld,
		    settings.MinT,
		    max(settings.MaxDistance, settings.MinT),
		    settings.RayFlags,
		    settings.InstanceMask);
	}

	RayTracingTraceResult TraceSurfaceRay(
	    RayTracingPathSurface surface,
	    float3 directionWorld,
	    TraceSettings settings,
	    out float3 rayOriginWorld)
	{
		rayOriginWorld = ComputeRayOrigin(surface, directionWorld, settings);
		return TraceSceneRay(rayOriginWorld, directionWorld, settings);
	}
}

#endif

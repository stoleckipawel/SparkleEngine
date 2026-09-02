#ifndef SPARKLE_RAY_TRACED_SHADOW_TRACE_HLSLI
#define SPARKLE_RAY_TRACED_SHADOW_TRACE_HLSLI

#include "/Engine/RayTracing/Shadows/RayTracedShadowSemantics.hlsli"
#include "/Engine/RayTracing/RayTracingSceneTlasTrace.hlsli"

namespace RayTracedShadows
{
	ShadowVisibilitySample TraceShadowRay(RayTracedShadowRequest request)
	{
		const RayTracingTraceResult trace = RayTracingSceneTlas::TraceRayQueryWithAlphaTest(request.OriginWorld,
		                                                                                    request.DirectionWorld,
		                                                                                    MinimumShadowTMin,
		                                                                                    request.MaxDistance,
		                                                                                    ShadowRayFlags,
		                                                                                    ShadowInstanceMask);
		return ResolveTrace(trace.Hit, trace.RayT, request.MaxDistance);
	}
}

#endif

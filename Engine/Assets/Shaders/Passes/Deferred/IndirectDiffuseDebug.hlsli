#pragma once

#include "RayTracing/RayTracingHitDebug.hlsli"
#include "RayTracing/RayTracingPathSample.hlsli"

namespace IndirectDiffuseDebug
{
	static const uint Off = 0u;
	static const uint HitMask = 1u;
	static const uint HitDistance = 2u;
	static const uint SampleDirection = 3u;
	static const uint SamplePdf = 4u;
	static const uint HitRadiance = 5u;
	static const uint FinalContribution = 6u;
	static const uint HitNormal = 7u;
	static const uint MaterialBaseColor = 8u;
	static const uint MissSkyRadiance = 9u;
	static const uint RejectionReason = 10u;

	float3 BuildColor(
	    uint debugMode,
	    RayTracingPathSample::DirectionSample sample,
	    RayTracingPathSample::LightingResult lighting,
	    float maxDistance)
	{
		if (debugMode == HitMask)
		{
			if (lighting.Hit)
			{
				return 1.0f.xxx;
			}
			return lighting.TraceHit ? float3(1.0f, 0.0f, 0.0f) : float3(0.0f, 0.2f, 1.0f);
		}
		if (debugMode == HitDistance)
		{
			return saturate(lighting.HitDistance / max(maxDistance, 1.0e-4f)).xxx;
		}
		if (debugMode == SampleDirection)
		{
			return saturate(sample.DirectionWorld * 0.5f + 0.5f);
		}
		if (debugMode == SamplePdf)
		{
			return saturate(sample.Pdf * 3.14159265359f).xxx;
		}
		if (debugMode == HitRadiance)
		{
			return RayTracingDebugPreviewHdr(lighting.IncidentRadiance);
		}
		if (debugMode == FinalContribution)
		{
			return RayTracingDebugPreviewHdr(lighting.Contribution * IndirectDiffuseIntensity);
		}
		if (debugMode == HitNormal)
		{
			return lighting.Hit ? saturate(lighting.HitNormalWorld * 0.5f + 0.5f) : 0.0f.xxx;
		}
		if (debugMode == MaterialBaseColor)
		{
			return lighting.Hit ? saturate(lighting.MaterialBaseColor) : 0.0f.xxx;
		}
		if (debugMode == MissSkyRadiance)
		{
			return lighting.TraceHit ? 0.0f.xxx : RayTracingDebugPreviewHdr(lighting.MissRadiance);
		}
		if (debugMode == RejectionReason)
		{
			return RayTracingDebugReasonColor(lighting.SurfaceRejectionReason);
		}

		return lighting.Contribution * IndirectDiffuseIntensity;
	}
}

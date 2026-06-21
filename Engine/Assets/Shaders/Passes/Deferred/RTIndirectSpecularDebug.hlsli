#pragma once

#include "RayTracing/RayTracingHitDebug.hlsli"

float3 BuildRTIndirectSpecularDebugColor(
    RayTracingTraceResult trace,
    RayTracingHitSurfaceData hitSurface,
    RTIndirectSpecularSampleResult sample,
    RTIndirectSpecularResolvedContribution resolved,
    float3 mirrorDirectionWorld)
{
	if (RTIndirectSpecularDebugMode == RTIndirectSpecularDebugMirrorDirection)
	{
		return mirrorDirectionWorld * 0.5f + 0.5f;
	}
	if (RTIndirectSpecularDebugMode == RTIndirectSpecularDebugSampleDirection)
	{
		return sample.DirectionWorld * 0.5f + 0.5f;
	}
	if (RTIndirectSpecularDebugMode == RTIndirectSpecularDebugSamplePdf)
	{
		return saturate(sample.Pdf * 4.0f).xxx;
	}
	if (RTIndirectSpecularDebugMode == RTIndirectSpecularDebugSampleThroughput)
	{
		return saturate(sample.ThroughputNoF).xxx;
	}
	if (RTIndirectSpecularDebugMode == RTIndirectSpecularDebugHitRadiance)
	{
		return RayTracingDebugPreviewHdr(resolved.HitRadiance);
	}
	if (RTIndirectSpecularDebugMode == RTIndirectSpecularDebugFinalContribution)
	{
		return RayTracingDebugPreviewHdr(resolved.FinalContribution);
	}
	if (RTIndirectSpecularDebugMode == RayTracingDebugModes::MaterialBaseColor)
	{
		return hitSurface.Valid ? hitSurface.BaseColor : RayTracingDebugReasonColor(hitSurface.RejectionReason);
	}
	if (RTIndirectSpecularDebugMode == RayTracingDebugModes::MaterialRoughnessMetallic)
	{
		return hitSurface.Valid ? float3(hitSurface.Roughness, hitSurface.Metallic, 0.0f)
		                        : RayTracingDebugReasonColor(hitSurface.RejectionReason);
	}
	if (RTIndirectSpecularDebugMode == RayTracingDebugModes::MaterialEmissive)
	{
		return hitSurface.Valid ? RayTracingDebugPreviewHdr(hitSurface.EmissiveColor)
		                        : RayTracingDebugReasonColor(hitSurface.RejectionReason);
	}
	if (RTIndirectSpecularDebugMode == RayTracingDebugModes::HitTangent)
	{
		return hitSurface.Valid ? hitSurface.TangentWorld * 0.5f + 0.5f : RayTracingDebugReasonColor(hitSurface.RejectionReason);
	}
	if (RTIndirectSpecularDebugMode == RayTracingDebugModes::HitBitangent)
	{
		return hitSurface.Valid ? hitSurface.BitangentWorld * 0.5f + 0.5f : RayTracingDebugReasonColor(hitSurface.RejectionReason);
	}
	if (RTIndirectSpecularDebugMode == RayTracingDebugModes::HitNormalTangent)
	{
		return hitSurface.Valid ? hitSurface.NormalTangent * 0.5f + 0.5f : RayTracingDebugReasonColor(hitSurface.RejectionReason);
	}
	if (RTIndirectSpecularDebugMode == RayTracingDebugModes::HitSampledNormal)
	{
		return hitSurface.Valid ? hitSurface.NormalWorld * 0.5f + 0.5f : RayTracingDebugReasonColor(hitSurface.RejectionReason);
	}
	if (RTIndirectSpecularDebugMode == RayTracingDebugModes::AlphaAcceptedRejected)
	{
		if (!trace.AlphaCandidateSeen)
		{
			return 0.0f.xxx;
		}
		return trace.AlphaCandidateAccepted ? float3(0.0f, 0.85f, 0.2f) : float3(0.05f, 0.25f, 1.0f);
	}
	if (RTIndirectSpecularDebugMode == RayTracingDebugModes::AlphaSample)
	{
		return trace.AlphaCandidateSeen ? saturate(trace.AlphaCandidateValue).xxx : 0.0f.xxx;
	}
	if (RTIndirectSpecularDebugMode == RayTracingDebugModes::AlphaCutoff)
	{
		return trace.AlphaCandidateSeen ? saturate(trace.AlphaCandidateCutoff).xxx : 0.0f.xxx;
	}
	if (RTIndirectSpecularDebugMode == RayTracingDebugModes::HitRejectionReason)
	{
		return RayTracingDebugReasonColor(hitSurface.RejectionReason);
	}
	if (!trace.Hit)
	{
		return 0.0f.xxx;
	}
	if (RTIndirectSpecularDebugMode == RayTracingDebugModes::HitMask)
	{
		return 1.0f.xxx;
	}
	if (RTIndirectSpecularDebugMode == RayTracingDebugModes::HitDistance)
	{
		const float normalizedDistance = saturate(trace.RayT / max(RTIndirectSpecularMaxDistance, RTIndirectSpecularMinimumTMin));
		return lerp(float3(0.05f, 0.25f, 1.0f), float3(1.0f, 0.85f, 0.05f), normalizedDistance);
	}
	if (RTIndirectSpecularDebugMode == RayTracingDebugModes::HitUV)
	{
		return hitSurface.Valid ? float3(frac(hitSurface.TexCoord0), 0.0f) : RayTracingDebugReasonColor(hitSurface.RejectionReason);
	}
	if (RTIndirectSpecularDebugMode == RayTracingDebugModes::HitNormal)
	{
		return hitSurface.Valid ? hitSurface.NormalWorld * 0.5f + 0.5f : RayTracingDebugReasonColor(hitSurface.RejectionReason);
	}
	if (RTIndirectSpecularDebugMode == RayTracingDebugModes::MaterialId)
	{
		return hitSurface.Valid ? HashIdColor(hitSurface.MaterialSlot, 0u) : RayTracingDebugReasonColor(hitSurface.RejectionReason);
	}
	if (RTIndirectSpecularDebugMode == RayTracingDebugModes::GeometryClass)
	{
		return RayTracingDebugGeometryClassColor(hitSurface.GeometryFlags);
	}
	const float3 idColor = HashIdColor(trace.InstanceId, trace.PrimitiveIndex);
	const float distanceShade = 1.0f / (1.0f + trace.RayT * 0.02f);
	return idColor * distanceShade;
}

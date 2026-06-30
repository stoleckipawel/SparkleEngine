#pragma once

#include "RayTracing/RayTracingHitDebug.hlsli"

float3 BuildIndirectSpecularDebugColor(
    RayTracingTraceResult trace,
    RayTracingHitSurfaceData hitSurface,
    BRDF::SpecularSampling::LobeSample sample,
    IndirectSpecularResolvedContribution resolved,
    float3 mirrorDirectionWorld)
{
	if (IndirectSpecularDebugMode == IndirectSpecularDebugMirrorDirection)
	{
		return mirrorDirectionWorld * 0.5f + 0.5f;
	}
	if (IndirectSpecularDebugMode == IndirectSpecularDebugSampleDirection)
	{
		return sample.DirectionWorld * 0.5f + 0.5f;
	}
	if (IndirectSpecularDebugMode == IndirectSpecularDebugSamplePdf)
	{
		return saturate(sample.Pdf * 4.0f).xxx;
	}
	if (IndirectSpecularDebugMode == IndirectSpecularDebugSampleThroughput)
	{
		return saturate(sample.Throughput);
	}
	if (IndirectSpecularDebugMode == IndirectSpecularDebugHitRadiance)
	{
		return RayTracingDebugPreviewHdr(resolved.HitRadiance);
	}
	if (IndirectSpecularDebugMode == IndirectSpecularDebugFinalContribution)
	{
		return RayTracingDebugPreviewHdr(resolved.FinalContribution);
	}
	if (IndirectSpecularDebugMode == RayTracingDebugModes::MaterialBaseColor)
	{
		return hitSurface.Valid ? hitSurface.BaseColor : RayTracingDebugReasonColor(hitSurface.RejectionReason);
	}
	if (IndirectSpecularDebugMode == RayTracingDebugModes::MaterialRoughnessMetallic)
	{
		return hitSurface.Valid ? float3(hitSurface.Roughness, hitSurface.Metallic, 0.0f)
		                        : RayTracingDebugReasonColor(hitSurface.RejectionReason);
	}
	if (IndirectSpecularDebugMode == RayTracingDebugModes::MaterialEmissive)
	{
		return hitSurface.Valid ? RayTracingDebugPreviewHdr(hitSurface.EmissiveColor)
		                        : RayTracingDebugReasonColor(hitSurface.RejectionReason);
	}
	if (IndirectSpecularDebugMode == RayTracingDebugModes::HitTangent)
	{
		return hitSurface.Valid ? hitSurface.TangentWorld * 0.5f + 0.5f : RayTracingDebugReasonColor(hitSurface.RejectionReason);
	}
	if (IndirectSpecularDebugMode == RayTracingDebugModes::HitBitangent)
	{
		return hitSurface.Valid ? hitSurface.BitangentWorld * 0.5f + 0.5f : RayTracingDebugReasonColor(hitSurface.RejectionReason);
	}
	if (IndirectSpecularDebugMode == RayTracingDebugModes::HitNormalTangent)
	{
		return hitSurface.Valid ? hitSurface.NormalTangent * 0.5f + 0.5f : RayTracingDebugReasonColor(hitSurface.RejectionReason);
	}
	if (IndirectSpecularDebugMode == RayTracingDebugModes::HitSampledNormal)
	{
		return hitSurface.Valid ? hitSurface.NormalWorld * 0.5f + 0.5f : RayTracingDebugReasonColor(hitSurface.RejectionReason);
	}
	if (IndirectSpecularDebugMode == RayTracingDebugModes::AlphaAcceptedRejected)
	{
		if (!trace.AlphaCandidateSeen)
		{
			return 0.0f.xxx;
		}
		return trace.AlphaCandidateAccepted ? float3(0.0f, 0.85f, 0.2f) : float3(0.05f, 0.25f, 1.0f);
	}
	if (IndirectSpecularDebugMode == RayTracingDebugModes::AlphaSample)
	{
		return trace.AlphaCandidateSeen ? saturate(trace.AlphaCandidateValue).xxx : 0.0f.xxx;
	}
	if (IndirectSpecularDebugMode == RayTracingDebugModes::AlphaCutoff)
	{
		return trace.AlphaCandidateSeen ? saturate(trace.AlphaCandidateCutoff).xxx : 0.0f.xxx;
	}
	if (IndirectSpecularDebugMode == RayTracingDebugModes::HitRejectionReason)
	{
		return RayTracingDebugReasonColor(hitSurface.RejectionReason);
	}
	if (!trace.Hit)
	{
		return 0.0f.xxx;
	}
	if (IndirectSpecularDebugMode == RayTracingDebugModes::HitMask)
	{
		return 1.0f.xxx;
	}
	if (IndirectSpecularDebugMode == RayTracingDebugModes::HitDistance)
	{
		const float normalizedDistance = saturate(trace.RayT / max(IndirectSpecularMaxDistance, IndirectSpecularMinimumTMin));
		return lerp(float3(0.05f, 0.25f, 1.0f), float3(1.0f, 0.85f, 0.05f), normalizedDistance);
	}
	if (IndirectSpecularDebugMode == RayTracingDebugModes::HitUV)
	{
		return hitSurface.Valid ? float3(frac(hitSurface.TexCoord0), 0.0f) : RayTracingDebugReasonColor(hitSurface.RejectionReason);
	}
	if (IndirectSpecularDebugMode == RayTracingDebugModes::HitNormal)
	{
		return hitSurface.Valid ? hitSurface.NormalWorld * 0.5f + 0.5f : RayTracingDebugReasonColor(hitSurface.RejectionReason);
	}
	if (IndirectSpecularDebugMode == RayTracingDebugModes::MaterialId)
	{
		return hitSurface.Valid ? HashIdColor(hitSurface.MaterialSlot, 0u) : RayTracingDebugReasonColor(hitSurface.RejectionReason);
	}
	if (IndirectSpecularDebugMode == RayTracingDebugModes::GeometryClass)
	{
		return RayTracingDebugGeometryClassColor(hitSurface.GeometryFlags);
	}
	const float3 idColor = HashIdColor(trace.InstanceId, trace.PrimitiveIndex);
	const float distanceShade = 1.0f / (1.0f + trace.RayT * 0.02f);
	return idColor * distanceShade;
}

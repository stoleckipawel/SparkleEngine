#pragma once

#include "/Engine/Common/Hash.hlsli"
#include "/Engine/RayTracing/RayTracingDebugModes.hlsli"
#include "/Engine/RayTracing/RayTracingHitSurface.hlsli"

float3 RayTracingDebugReasonColor(uint reason)
{
	if (reason == RayTracingHitSurface::ReasonNone)
	{
		return float3(0.1f, 1.0f, 0.25f);
	}
	if (reason == RayTracingHitSurface::ReasonNoHit)
	{
		return 0.0f.xxx;
	}
	if (reason == RayTracingHitSurface::ReasonMissingMeshHitData)
	{
		return float3(1.0f, 0.55f, 0.0f);
	}
	if (reason == RayTracingHitSurface::ReasonInvalidMaterial)
	{
		return float3(1.0f, 0.0f, 0.8f);
	}
	if (reason == RayTracingHitSurface::ReasonInvalidPrimitive || reason == RayTracingHitSurface::ReasonInvalidVertexIndex)
	{
		return float3(1.0f, 1.0f, 0.0f);
	}
	if (reason == RayTracingHitSurface::ReasonOneSidedBackface)
	{
		return float3(0.0f, 0.85f, 0.95f);
	}
	if (reason == RayTracingHitSurface::ReasonAlphaRejected)
	{
		return float3(0.05f, 0.25f, 1.0f);
	}
	return float3(0.2f, 0.65f, 1.0f);
}

float3 RayTracingDebugGeometryClassColor(uint geometryFlags)
{
	if ((geometryFlags & RayTracingHitSurface::GeometryFlagSkinnedMesh) != 0u)
	{
		return float3(0.65f, 0.2f, 1.0f);
	}
	if ((geometryFlags & RayTracingHitSurface::GeometryFlagAlphaTested) != 0u ||
	    (geometryFlags & RayTracingHitSurface::GeometryFlagAlphaBlended) != 0u)
	{
		return float3(1.0f, 0.2f, 0.05f);
	}
	if ((geometryFlags & RayTracingHitSurface::GeometryFlagDoubleSided) != 0u)
	{
		return float3(0.05f, 0.85f, 1.0f);
	}
	if ((geometryFlags & RayTracingHitSurface::GeometryFlagTexturedMaterial) != 0u)
	{
		return float3(0.1f, 0.95f, 0.45f);
	}
	if ((geometryFlags & RayTracingHitSurface::GeometryFlagStaticMesh) != 0u)
	{
		return float3(0.85f, 0.85f, 0.85f);
	}
	return float3(0.2f, 0.2f, 0.2f);
}

float3 RayTracingDebugPreviewHdr(float3 value)
{
	return value / (1.0f.xxx + value);
}

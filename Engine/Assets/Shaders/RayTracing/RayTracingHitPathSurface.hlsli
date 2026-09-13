#ifndef SPARKLE_RAY_TRACING_HIT_PATH_SURFACE_HLSLI
#define SPARKLE_RAY_TRACING_HIT_PATH_SURFACE_HLSLI

#include "/Engine/Resources/MeshInstanceShaderData.hlsli"

#include "/Engine/RayTracing/PathSurface.hlsli"
#include "/Engine/RayTracing/RayTracingHitData.hlsli"
#include "/Engine/RayTracing/RayTracingTraceResult.hlsli"

RayTracingPathSurface BuildStaticOpaqueLambertianPathSurface(RayTracingTraceResult trace, float3 outgoingWorld)
{
	RayTracingPathSurface surface = (RayTracingPathSurface)0;
	if (!trace.Hit)
	{
		return surface;
	}

	RayTracingHitInstance hitInstance = (RayTracingHitInstance)0;
	RayTracingHitMaterial material = (RayTracingHitMaterial)0;
	float3 barycentricWeights = 0.0f.xxx;
	uint3 vertexIndices = 0u.xxx;
	RayTracingHitVertex v0 = (RayTracingHitVertex)0;
	RayTracingHitVertex v1 = (RayTracingHitVertex)0;
	RayTracingHitVertex v2 = (RayTracingHitVertex)0;
	uint rejectionReason = RayTracingHitSurface::ReasonNone;
	if (!TryLoadRayTracingHitTriangle(trace.InstanceId,
	                                  trace.PrimitiveIndex,
	                                  trace.Barycentrics,
	                                  hitInstance,
	                                  material,
	                                  barycentricWeights,
	                                  vertexIndices,
	                                  v0,
	                                  v1,
	                                  v2,
	                                  rejectionReason))
	{
		return surface;
	}

	const MeshInstanceData meshInstance = MeshInstances[trace.InstanceId];
	if ((meshInstance.Flags & (MeshInstanceFlag_Skinned | MeshInstanceFlag_Morphed)) != 0u
	    || material.AlphaMode != RayTracingHitSurface::AlphaModeOpaque || material.TextureFlags != 0u || material.Metallic != 0.0f
	    || material.SubsurfaceStrength != 0.0f || !all(isfinite(material.BaseColor.rgb)) || any(material.BaseColor.rgb < 0.0f)
	    || any(material.BaseColor.rgb > 1.0f) || !all(isfinite(material.EmissiveColor)) || any(material.EmissiveColor < 0.0f))
	{
		return surface;
	}

	const float3 p0 = mul(float4(v0.Position, 1.0f), meshInstance.WorldMatrix).xyz;
	const float3 p1 = mul(float4(v1.Position, 1.0f), meshInstance.WorldMatrix).xyz;
	const float3 p2 = mul(float4(v2.Position, 1.0f), meshInstance.WorldMatrix).xyz;
	const float3 normalUnnormalized = cross(p1 - p0, p2 - p0);
	const float normalLengthSquared = dot(normalUnnormalized, normalUnnormalized);
	if (normalLengthSquared <= 0.0f)
	{
		return surface;
	}

	float3 normal = normalUnnormalized / sqrt(normalLengthSquared);
	if (!trace.FrontFace && (hitInstance.Flags & RayTracingHitSurface::InstanceFlagTwoSided) == 0u)
	{
		return surface;
	}
	if (dot(normal, outgoingWorld) <= 0.0f)
	{
		normal = -normal;
	}

	surface.PositionWorld = p0 * barycentricWeights.x + p1 * barycentricWeights.y + p2 * barycentricWeights.z;
	surface.NormalWorld = normal;
	surface.ViewDirWorld = outgoingWorld;
	surface.BaseColor = material.BaseColor.rgb;
	surface.EmissiveColor = material.EmissiveColor;
	surface.Roughness = 1.0f;
	surface.Metallic = 0.0f;
	surface.DielectricF0 = material.F0;
	surface.Valid =
	    all(isfinite(surface.PositionWorld)) && all(isfinite(surface.NormalWorld)) && dot(surface.NormalWorld, outgoingWorld) > 0.0f;
	return surface;
}

#endif

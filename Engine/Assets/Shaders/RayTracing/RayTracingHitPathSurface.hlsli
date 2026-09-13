#ifndef SPARKLE_RAY_TRACING_HIT_PATH_SURFACE_HLSLI
#define SPARKLE_RAY_TRACING_HIT_PATH_SURFACE_HLSLI

#include "/Engine/Resources/MeshInstanceShaderData.hlsli"

#include "/Engine/RayTracing/PathSurface.hlsli"
#include "/Engine/RayTracing/RayTracingHitData.hlsli"
#include "/Engine/RayTracing/RayTracingTraceResult.hlsli"

RayTracingPathSurface BuildStaticOpaquePathSurface(RayTracingTraceResult trace, float3 outgoingWorld)
{
	const RayTracingHitTriangle triangle =
	    LoadRayTracingHitTriangle(trace.InstanceId, trace.PrimitiveIndex, trace.Barycentrics);

	const MeshInstanceData meshInstance = MeshInstances[trace.InstanceId];
	const float3 p0 = mul(float4(triangle.V0.Position, 1.0f), meshInstance.WorldMatrix).xyz;
	const float3 p1 = mul(float4(triangle.V1.Position, 1.0f), meshInstance.WorldMatrix).xyz;
	const float3 p2 = mul(float4(triangle.V2.Position, 1.0f), meshInstance.WorldMatrix).xyz;
	const float3 normalUnnormalized = cross(p1 - p0, p2 - p0);
	const float normalLengthSquared = dot(normalUnnormalized, normalUnnormalized);
	float3 normal = normalUnnormalized / sqrt(normalLengthSquared);
	if (dot(normal, outgoingWorld) <= 0.0f)
	{
		normal = -normal;
	}

	RayTracingPathSurface surface = (RayTracingPathSurface)0;
	surface.PositionWorld = p0 * triangle.BarycentricWeights.x + p1 * triangle.BarycentricWeights.y
	                      + p2 * triangle.BarycentricWeights.z;
	surface.GeometricNormalWorld = normal;
	surface.NormalWorld = normal;
	surface.ViewDirWorld = outgoingWorld;
	surface.BaseColor = triangle.Material.BaseColor.rgb;
	surface.EmissiveColor = triangle.Material.EmissiveColor;
	surface.Roughness = triangle.Material.Roughness;
	surface.Metallic = triangle.Material.Metallic;
	surface.DielectricF0 = 0.04f;
	surface.InstanceId = trace.InstanceId;
	surface.PrimitiveIndex = trace.PrimitiveIndex;
	surface.EmissionTwoSided = (triangle.Instance.Flags & RayTracingHitSurface::InstanceFlagTwoSided) != 0u;
	return surface;
}

#endif

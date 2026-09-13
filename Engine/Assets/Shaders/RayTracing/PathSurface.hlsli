#ifndef SPARKLE_RAY_TRACING_PATH_SURFACE_HLSLI
#define SPARKLE_RAY_TRACING_PATH_SURFACE_HLSLI

#include "/Engine/RayTracing/RayTracingHitSurface.hlsli"

struct RayTracingPathSurface
{
	bool Valid;
	float3 PositionWorld;
	float PositionError;
	float3 GeometricNormalWorld;
	float3 NormalWorld;
	float3 ViewDirWorld;
	float3 BaseColor;
	float3 EmissiveColor;
	float Roughness;
	float Metallic;
	float DielectricF0;
	uint InstanceId;
	uint PrimitiveIndex;
	bool EmissionTwoSided;
};

RayTracingPathSurface BuildPrimaryRayTracingPathSurface(float3 positionWorld,
                                                        float3 normalWorld,
                                                        float3 viewDirWorld,
                                                        float3 baseColor,
                                                        float roughness,
                                                        float metallic,
                                                        float dielectricF0)
{
	RayTracingPathSurface surface;
	surface.Valid = true;
	surface.PositionWorld = positionWorld;
	surface.PositionError = 0x1.0p-23f * dot(abs(normalWorld), abs(positionWorld));
	surface.GeometricNormalWorld = normalWorld;
	surface.NormalWorld = normalWorld;
	surface.ViewDirWorld = viewDirWorld;
	surface.BaseColor = baseColor;
	surface.EmissiveColor = 0.0f.xxx;
	surface.Roughness = roughness;
	surface.Metallic = metallic;
	surface.DielectricF0 = dielectricF0;
	surface.InstanceId = 0xFFFFFFFFu;
	surface.PrimitiveIndex = 0xFFFFFFFFu;
	surface.EmissionTwoSided = false;
	return surface;
}

RayTracingPathSurface BuildHitRayTracingPathSurface(RayTracingHitSurfaceData hitSurface, float3 incomingRayDirectionWorld)
{
	RayTracingPathSurface surface;
	surface.Valid = hitSurface.Valid;
	surface.PositionWorld = hitSurface.PositionWorld;
	surface.PositionError = hitSurface.PositionError;
	surface.GeometricNormalWorld = hitSurface.GeometricNormalWorld;
	surface.NormalWorld = hitSurface.NormalWorld;
	surface.ViewDirWorld = normalize(-incomingRayDirectionWorld);
	surface.BaseColor = hitSurface.BaseColor;
	surface.EmissiveColor = hitSurface.EmissiveColor;
	surface.Roughness = hitSurface.Roughness;
	surface.Metallic = hitSurface.Metallic;
	surface.DielectricF0 = hitSurface.DielectricF0;
	surface.InstanceId = hitSurface.InstanceId;
	surface.PrimitiveIndex = hitSurface.PrimitiveIndex;
	surface.EmissionTwoSided = hitSurface.EmissionTwoSided;
	return surface;
}

#endif

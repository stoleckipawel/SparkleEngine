#ifndef SPARKLE_RAY_TRACING_PATH_SURFACE_HLSLI
#define SPARKLE_RAY_TRACING_PATH_SURFACE_HLSLI

#include "RayTracing/RayTracingMaterialHit.hlsli"

struct RayTracingPathSurface
{
	bool Valid;
	float3 PositionWorld;
	float3 NormalWorld;
	float3 ViewDirWorld;
	float3 BaseColor;
	float Roughness;
	float Metallic;
	float DielectricF0;
};

RayTracingPathSurface BuildPrimaryRayTracingPathSurface(
    float3 positionWorld,
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
	surface.NormalWorld = normalWorld;
	surface.ViewDirWorld = viewDirWorld;
	surface.BaseColor = baseColor;
	surface.Roughness = roughness;
	surface.Metallic = metallic;
	surface.DielectricF0 = dielectricF0;
	return surface;
}

RayTracingPathSurface BuildHitRayTracingPathSurface(
    RayTracingHitSurfaceData hitSurface,
    float3 incomingRayDirectionWorld)
{
	RayTracingPathSurface surface;
	surface.Valid = hitSurface.Valid;
	surface.PositionWorld = hitSurface.PositionWorld;
	surface.NormalWorld = hitSurface.NormalWorld;
	surface.ViewDirWorld = normalize(-incomingRayDirectionWorld);
	surface.BaseColor = hitSurface.BaseColor;
	surface.Roughness = hitSurface.Roughness;
	surface.Metallic = hitSurface.Metallic;
	surface.DielectricF0 = hitSurface.DielectricF0;
	return surface;
}

#endif

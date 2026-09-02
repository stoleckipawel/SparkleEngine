#ifndef SPARKLE_RAY_TRACING_GBUFFER_COMMON_HLSLI
#define SPARKLE_RAY_TRACING_GBUFFER_COMMON_HLSLI

#include "/Engine/Resources/ViewUniformData.hlsli"
#include "/Engine/Resources/ViewCameraUniformData.hlsli"
#include "/Engine/Resources/ViewTemporalUniformData.hlsli"

#include "/Engine/Debug/InstanceView.hlsli"
#include "/Engine/Geometry/ScreenSpace.hlsli"
#include "/Engine/Geometry/Transforms.hlsli"
#include "/Engine/Passes/GBuffer/GBufferPacking.hlsli"
#include "/Engine/Passes/GBuffer/MotionVector.hlsli"

cbuffer RayTracingGBufferConstants
{
	uint RayTracingHitInstanceCount;
	uint RayTracingHitMaterialCount;
};

#include "/Engine/RayTracing/RayTracingMaterialHit.hlsli"
#include "/Engine/RayTracing/RayTracingShaderTableLayout.hlsli"

RWTexture2D<float4> GBufferBaseColor;
RWTexture2D<float4> GBufferNormal;
RWTexture2D<float4> GBufferMaterial;
RWTexture2D<float4> GBufferEmissive;
RWTexture2D<float4> GBufferSubsurface;
RWTexture2D<float> GBufferDeviceZ;
RWTexture2D<float2> GBufferMotionVector;
RaytracingAccelerationStructure SceneTlas;

namespace RayTracingGBuffer
{
	static const uint CullFlags = RAY_FLAG_CULL_BACK_FACING_TRIANGLES;
	static const uint InstanceMask = 0xFFu;
	static const float MinimumT = 0.001f;

	struct PrimaryRay
	{
		float3 OriginWorld;
		float3 DirectionWorld;
		RayDesc Description;
	};

	PrimaryRay BuildPrimaryRay(uint2 pixelCoord)
	{
		PrimaryRay ray;
		ray.OriginWorld = Position;
		ray.DirectionWorld = ComputeSkyViewDirectionWorld(pixelCoord);
		ray.Description.Origin = ray.OriginWorld;
		ray.Description.Direction = normalize(ray.DirectionWorld);
		ray.Description.TMin = MinimumT;
		ray.Description.TMax = FarZ;
		return ray;
	}

	float ComputeDeviceZ(float3 positionWorld)
	{
		const float4 positionClip = PositionWorldToClip(float4(positionWorld, 1.0f));
		return positionClip.z / max(positionClip.w, 1.0e-6f);
	}

	float2 ComputeMotionVector(RayTracingHitSurfaceData surface)
	{
		const float4 currentClipPosition = PositionWorldToClip(float4(surface.PositionWorld, 1.0f));
		const float4 previousClipPosition = mul(float4(surface.PreviousPositionWorld, 1.0f), PreviousWorldToClipMatrix);
		return MotionVectors::Compute(currentClipPosition, previousClipPosition, ViewportSize);
	}

	void StoreMiss(uint2 pixelCoord)
	{
		GBufferBaseColor[pixelCoord] = GBufferPacking::PackSkyBaseColor();
		GBufferNormal[pixelCoord] = GBufferPacking::PackSkyNormal();
		GBufferMaterial[pixelCoord] = GBufferPacking::PackSkyMaterial();
		GBufferEmissive[pixelCoord] = GBufferPacking::PackSkyEmissive();
		GBufferSubsurface[pixelCoord] = GBufferPacking::PackSkySubsurface();
		GBufferDeviceZ[pixelCoord] = GBufferPacking::PackSkyDeviceZ();
		GBufferMotionVector[pixelCoord] = 0.0f.xx;
	}

	void StoreHit(uint2 pixelCoord, RayTracingHitSurfaceData surface)
	{
		const float3 baseColor = InstanceView::ApplyInstanceVisualization(surface.BaseColor, surface.GpuSceneSlot);
		GBufferBaseColor[pixelCoord] =
		    GBufferPacking::PackBaseColor(baseColor, surface.Alpha, surface.AlphaMode, RayTracingHitSurface::AlphaModeBlended);
		GBufferNormal[pixelCoord] = GBufferPacking::PackNormal(surface.NormalWorld);
		GBufferMaterial[pixelCoord] =
		    GBufferPacking::PackMaterial(surface.Metallic, surface.Roughness, surface.AmbientOcclusion, surface.DielectricF0);
		GBufferEmissive[pixelCoord] = GBufferPacking::PackEmissive(surface.EmissiveColor);
		GBufferSubsurface[pixelCoord] = GBufferPacking::PackSubsurface(surface.SubsurfaceColor, surface.SubsurfaceStrength);
		GBufferDeviceZ[pixelCoord] = ComputeDeviceZ(surface.PositionWorld);
		GBufferMotionVector[pixelCoord] = ComputeMotionVector(surface);
	}

	void StoreTraceResult(uint2 pixelCoord, RayTracingTraceResult trace, PrimaryRay ray)
	{
		if (!trace.Hit)
		{
			StoreMiss(pixelCoord);
			return;
		}
		const RayTracingHitSurfaceData surface = ReconstructRayTracingHitSurface(trace, ray.OriginWorld, ray.DirectionWorld);
		if (!surface.Valid)
		{
			StoreMiss(pixelCoord);
			return;
		}
		StoreHit(pixelCoord, surface);
	}
}

#endif

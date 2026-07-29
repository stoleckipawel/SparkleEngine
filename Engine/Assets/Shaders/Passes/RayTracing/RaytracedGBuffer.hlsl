#include "Resources/ConstantBuffers.hlsli"
#include "Debug/InstanceView.hlsli"
#include "Geometry/ScreenSpace.hlsli"
#include "Geometry/Transforms.hlsli"
#include "Passes/Deferred/GBufferPacking.hlsli"
#include "Passes/Deferred/MotionVector.hlsli"

RWTexture2D<float4> GBufferBaseColor;
RWTexture2D<float4> GBufferNormal;
RWTexture2D<float4> GBufferMaterial;
RWTexture2D<float4> GBufferEmissive;
RWTexture2D<float4> GBufferSubsurface;
RWTexture2D<float> GBufferDeviceZ;
RWTexture2D<float2> GBufferMotionVector;

cbuffer RaytracedGBufferUniformData
{
	uint RayTracingHitInstanceCount;
	uint RayTracingHitMaterialCount;
	uint RaytracedGBufferPadding0;
	uint RaytracedGBufferPadding1;
};

#include "RayTracing/PathTrace.hlsli"

static const uint RaytracedGBufferRayFlags = RAY_FLAG_SKIP_CLOSEST_HIT_SHADER | RAY_FLAG_CULL_BACK_FACING_TRIANGLES;
static const uint RaytracedGBufferInstanceMask = 0xFFu;
static const float RaytracedGBufferMinimumTMin = 0.001f;

float ComputeRaytracedGBufferDeviceZ(float3 positionWorld)
{
	const float4 positionClip = PositionWorldToClip(float4(positionWorld, 1.0f));
	return positionClip.z / max(positionClip.w, 1.0e-6f);
}

float2 ComputeRaytracedGBufferMotionVector(RayTracingHitSurfaceData surface)
{
	const float4 currentClipPosition = PositionWorldToClip(float4(surface.PositionWorld, 1.0f));
	const float4 previousClipPosition = mul(float4(surface.PreviousPositionWorld, 1.0f), PreviousWorldToClipMatrix);
	return MotionVectors::Compute(currentClipPosition, previousClipPosition, ViewportSize);
}

void StoreRaytracedGBufferMiss(uint2 pixelCoord)
{
	GBufferBaseColor[pixelCoord] = GBufferPacking::PackSkyBaseColor();
	GBufferNormal[pixelCoord] = GBufferPacking::PackSkyNormal();
	GBufferMaterial[pixelCoord] = GBufferPacking::PackSkyMaterial();
	GBufferEmissive[pixelCoord] = GBufferPacking::PackSkyEmissive();
	GBufferSubsurface[pixelCoord] = GBufferPacking::PackSkySubsurface();
	GBufferDeviceZ[pixelCoord] = GBufferPacking::PackSkyDeviceZ();
}

void StoreRaytracedGBufferHit(uint2 pixelCoord, RayTracingHitSurfaceData surface)
{
	const float3 baseColor = InstanceView::ApplyInstanceVisualization(surface.BaseColor, surface.GpuSceneSlot);

	GBufferBaseColor[pixelCoord] = GBufferPacking::PackBaseColor(baseColor, surface.Alpha, surface.AlphaMode, RayTracingHitSurface::AlphaModeBlended);
	GBufferNormal[pixelCoord] = GBufferPacking::PackNormal(surface.NormalWorld);
	GBufferMaterial[pixelCoord] = GBufferPacking::PackMaterial(surface.Metallic, surface.Roughness, surface.AmbientOcclusion, surface.DielectricF0);
	GBufferEmissive[pixelCoord] = GBufferPacking::PackEmissive(surface.EmissiveColor);
	GBufferSubsurface[pixelCoord] = GBufferPacking::PackSubsurface(surface.SubsurfaceColor, surface.SubsurfaceStrength);
	GBufferDeviceZ[pixelCoord] = ComputeRaytracedGBufferDeviceZ(surface.PositionWorld);
	GBufferMotionVector[pixelCoord] = ComputeRaytracedGBufferMotionVector(surface);
}

[numthreads(8, 8, 1)] void main(uint3 dispatchThreadId : SV_DispatchThreadID)
{
	uint width = 0;
	uint height = 0;
	GBufferBaseColor.GetDimensions(width, height);

	if (dispatchThreadId.x >= width || dispatchThreadId.y >= height)
	{
		return;
	}

	const uint2 pixelCoord = dispatchThreadId.xy;
	RayTracingPathTrace::TraceSettings traceSettings;
	traceSettings.NormalBias = 0.0f;
	traceSettings.MaxDistance = Camera.FarZ;
	traceSettings.MinT = RaytracedGBufferMinimumTMin;
	traceSettings.RayFlags = RaytracedGBufferRayFlags;
	traceSettings.InstanceMask = RaytracedGBufferInstanceMask;

	const float3 rayOriginWorld = Camera.Position;
	const float3 rayDirectionWorld = ComputeSkyViewDirectionWorld(pixelCoord);
	const RayTracingTraceResult primaryTrace =
	    RayTracingPathTrace::TraceSceneRay(rayOriginWorld, rayDirectionWorld, traceSettings);
	if (!primaryTrace.Hit)
	{
		StoreRaytracedGBufferMiss(pixelCoord);
		return;
	}

	const RayTracingHitSurfaceData surface =
	    ReconstructRayTracingHitSurface(primaryTrace, rayOriginWorld, rayDirectionWorld);
	if (!surface.Valid)
	{
		StoreRaytracedGBufferMiss(pixelCoord);
		return;
	}

	StoreRaytracedGBufferHit(pixelCoord, surface);
}

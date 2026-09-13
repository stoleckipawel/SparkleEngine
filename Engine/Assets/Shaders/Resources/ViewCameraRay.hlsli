#ifndef SPARKLE_RESOURCES_VIEW_CAMERA_RAY_HLSLI
#define SPARKLE_RESOURCES_VIEW_CAMERA_RAY_HLSLI

#include "/Engine/Resources/ViewCameraUniformData.hlsli"

struct ViewCameraRay
{
	float3 OriginWorld;
	float3 DirectionWorld;
	float TMin;
	float TMax;
	bool Valid;
};

ViewCameraRay BuildPerspectiveViewCameraRay(uint2 pixelCoord, uint2 extent, float2 filmSample)
{
	ViewCameraRay ray = (ViewCameraRay)0;
	const float2 rasterUv = (float2(pixelCoord) + filmSample) / float2(extent);
	const float2 ndc = float2(2.0f * rasterUv.x - 1.0f, 1.0f - 2.0f * rasterUv.y);
	const float4 nearView = mul(float4(ndc, 1.0f, 1.0f), InvProjectionMTX);
	if (nearView.w == 0.0f)
	{
		return ray;
	}

	const float3 directionWorld = normalize(mul(float4(nearView.xyz / nearView.w, 0.0f), InvViewMTX).xyz);
	const float forwardDistanceScale = dot(directionWorld, Direction);
	if (forwardDistanceScale <= 0.0f)
	{
		return ray;
	}

	ray.OriginWorld = Position;
	ray.DirectionWorld = directionWorld;
	ray.TMin = NearZ / forwardDistanceScale;
	ray.TMax = FarZ / forwardDistanceScale;
	ray.Valid = ray.TMin > 0.0f && ray.TMax > ray.TMin;
	return ray;
}

#endif

#pragma once

struct RayTracingHitSurfaceData
{
	bool Valid;
	float3 PositionWorld;
	float3 PreviousPositionWorld;
	float3 NormalWorld;
	float3 TangentWorld;
	float3 BitangentWorld;
	float3 NormalTangent;
	float TangentSign;
	float2 TexCoord0;
	uint MaterialSlot;
	uint GeometryFlags;
	uint RejectionReason;
	float3 BaseColor;
	float3 EmissiveColor;
	float3 SubsurfaceColor;
	float Roughness;
	float Metallic;
	float DielectricF0;
	float AmbientOcclusion;
	float Alpha;
	float SubsurfaceStrength;
	uint AlphaMode;
	uint GpuSceneSlot;
};

namespace RayTracingHitSurface
{
	static const uint AlphaModeOpaque = 0u;
	static const uint AlphaModeTested = 1u;
	static const uint AlphaModeBlended = 2u;

	static const uint InstanceFlagValid = 1u << 0u;
	static const uint InstanceFlagTwoSided = 1u << 2u;

	static const uint GeometryFlagStaticMesh = 1u << 0u;
	static const uint GeometryFlagSkinnedMesh = 1u << 1u;
	static const uint GeometryFlagAlphaTested = 1u << 2u;
	static const uint GeometryFlagAlphaBlended = 1u << 3u;
	static const uint GeometryFlagTexturedMaterial = 1u << 4u;
	static const uint GeometryFlagDoubleSided = 1u << 5u;
	static const uint MaterialFlagEmissive = 1u << 5u;

	static const uint ReasonNone = 0u;
	static const uint ReasonNoHit = 1u;
	static const uint ReasonInvalidHitData = 2u;
	static const uint ReasonInstanceOutOfRange = 3u;
	static const uint ReasonInvalidInstance = 4u;
	static const uint ReasonInvalidMaterial = 5u;
	static const uint ReasonMissingMeshHitData = 8u;
	static const uint ReasonInvalidPrimitive = 9u;
	static const uint ReasonInvalidVertexIndex = 10u;
	static const uint ReasonOneSidedBackface = 11u;
	static const uint ReasonAlphaRejected = 13u;
}

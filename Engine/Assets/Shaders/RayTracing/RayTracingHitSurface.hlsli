#pragma once

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

	static const uint ReasonNone = 0u;
	static const uint ReasonNoHit = 1u;
	static const uint ReasonHitDataUnavailable = 2u;
	static const uint ReasonInstanceOutOfRange = 3u;
	static const uint ReasonInvalidInstance = 4u;
	static const uint ReasonInvalidMaterial = 5u;
	static const uint ReasonMissingMeshHitData = 8u;
	static const uint ReasonInvalidPrimitive = 9u;
	static const uint ReasonInvalidVertexIndex = 10u;
	static const uint ReasonOneSidedBackface = 11u;
	static const uint ReasonAlphaRejected = 13u;
}

#pragma once

namespace RayTracingDebugModes
{
	static const uint Off = 0u;
	static const uint HitMask = 1u;
	static const uint HitDistance = 2u;
	static const uint HitUV = 4u;
	static const uint HitNormal = 5u;
	static const uint MaterialId = 6u;
	static const uint GeometryClass = 7u;
	static const uint HitRejectionReason = 8u;
	static const uint MaterialBaseColor = 15u;
	static const uint MaterialRoughnessMetallic = 16u;
	static const uint MaterialEmissive = 17u;
	static const uint HitTangent = 20u;
	static const uint HitBitangent = 21u;
	static const uint HitNormalTangent = 22u;
	static const uint HitSampledNormal = 23u;
	static const uint AlphaAcceptedRejected = 24u;
	static const uint AlphaSample = 25u;
	static const uint AlphaCutoff = 26u;
}

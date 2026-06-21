#pragma once

#include <cstdint>

enum class RTIndirectSpecularDebugMode : std::uint32_t
{
	Off = 0u,
	HitMask = 1u,
	HitDistance = 2u,
	MirrorDirection = 3u,
	HitUV = 4u,
	HitNormal = 5u,
	MaterialId = 6u,
	GeometryClass = 7u,
	FallbackReason = 8u,
	AlphaPolicy = 9u,
	SampleDirection = 10u,
	SamplePdf = 11u,
	SampleThroughput = 12u,
	HitRadiance = 13u,
	FinalContribution = 14u,
	MaterialTextureBaseColor = 15u,
	MaterialTextureRoughnessMetallic = 16u,
	MaterialTextureEmissive = 17u,
	MaterialTextureMip = 18u,
	MaterialTextureInvalidDescriptor = 19u,
	HitTangent = 20u,
	HitBitangent = 21u,
	HitNormalTangent = 22u,
	HitSampledNormal = 23u,
};

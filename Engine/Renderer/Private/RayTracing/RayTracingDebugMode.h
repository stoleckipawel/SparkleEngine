#pragma once

#include <cstdint>

namespace RayTracingDebugMode
{
	inline constexpr std::uint32_t Off = 0u;
	inline constexpr std::uint32_t HitMask = 1u;
	inline constexpr std::uint32_t HitDistance = 2u;
	inline constexpr std::uint32_t HitUV = 4u;
	inline constexpr std::uint32_t HitNormal = 5u;
	inline constexpr std::uint32_t MaterialId = 6u;
	inline constexpr std::uint32_t GeometryClass = 7u;
	inline constexpr std::uint32_t HitRejectionReason = 8u;
	inline constexpr std::uint32_t MaterialBaseColor = 15u;
	inline constexpr std::uint32_t MaterialRoughnessMetallic = 16u;
	inline constexpr std::uint32_t MaterialEmissive = 17u;
	inline constexpr std::uint32_t HitTangent = 20u;
	inline constexpr std::uint32_t HitBitangent = 21u;
	inline constexpr std::uint32_t HitNormalTangent = 22u;
	inline constexpr std::uint32_t HitSampledNormal = 23u;
	inline constexpr std::uint32_t AlphaAcceptedRejected = 24u;
	inline constexpr std::uint32_t AlphaSample = 25u;
	inline constexpr std::uint32_t AlphaCutoff = 26u;
}

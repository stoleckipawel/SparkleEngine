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
};

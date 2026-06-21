#pragma once

#include <cstdint>

enum class RTIndirectSpecularDebugMode : std::uint32_t
{
	Off = 0u,
	HitMask = 1u,
	HitDistance = 2u,
	MirrorDirection = 3u,
};


#pragma once

#include <cstdint>

struct PathTracedLightingSettings final
{
	std::uint32_t SamplesPerPixel = 64u;
	std::uint32_t BounceCount = 8u;
	float NormalBias = 0.01f;
	float MaxDistance = 100000.0f;
};

PathTracedLightingSettings BuildPathTracedLightingSettings() noexcept;

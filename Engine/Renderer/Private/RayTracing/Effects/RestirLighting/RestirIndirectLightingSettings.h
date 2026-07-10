#pragma once

#include <cstdint>

struct RestirIndirectLightingSettings final
{
	std::uint32_t BounceCount = 2u;
	float NormalBias = 0.01f;
	float MaxDistance = 100000.0f;
};

RestirIndirectLightingSettings BuildRestirIndirectLightingSettings() noexcept;

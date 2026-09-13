#pragma once

#include <cstdint>

struct RestirIndirectLightingSettings final
{
	std::uint32_t BounceCount = 2u;
};

RestirIndirectLightingSettings BuildRestirIndirectLightingSettings() noexcept;

#pragma once

#include <cstdint>

enum class RTIndirectSpecularSampleMode : std::uint32_t
{
	Mirror = 0u,
	StochasticGGX = 1u,
};

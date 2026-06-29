#pragma once

#include <cstdint>

struct ToneMappingUniformData final
{
	std::uint32_t ToneMapper = 1u;
	std::uint32_t ToneMappingPadding0 = 0u;
	std::uint32_t ToneMappingPadding1 = 0u;
	std::uint32_t ToneMappingPadding2 = 0u;
};

static_assert(sizeof(ToneMappingUniformData) == 16);

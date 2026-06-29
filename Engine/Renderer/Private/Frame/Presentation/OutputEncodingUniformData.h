#pragma once

#include <cstdint>

struct OutputEncodingUniformData final
{
	std::uint32_t OutputColorEncoding = 1u;
	std::uint32_t OutputEncodingPadding0 = 0u;
	std::uint32_t OutputEncodingPadding1 = 0u;
	std::uint32_t OutputEncodingPadding2 = 0u;
};

static_assert(sizeof(OutputEncodingUniformData) == 16);

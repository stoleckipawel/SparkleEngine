#pragma once

#include <cstdint>
#include <type_traits>

struct ReferenceLightingAccumulationUniformData
{
	std::uint32_t SamplesFrame = 1u;
	std::uint32_t HistoryValid = 0u;
	std::uint32_t Padding1 = 0u;
	std::uint32_t Padding2 = 0u;
};

static_assert(std::is_standard_layout_v<ReferenceLightingAccumulationUniformData>);
static_assert(std::is_trivially_copyable_v<ReferenceLightingAccumulationUniformData>);
static_assert(sizeof(ReferenceLightingAccumulationUniformData) == 16);

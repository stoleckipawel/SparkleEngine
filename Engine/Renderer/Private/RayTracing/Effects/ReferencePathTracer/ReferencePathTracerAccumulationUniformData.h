#pragma once

#include <cstdint>
#include <type_traits>

struct ReferencePathTracerAccumulationUniformData
{
	std::uint32_t SamplesPerFrame = 1u;
	std::uint32_t HistoryValid = 0u;
	std::uint32_t Padding1 = 0u;
	std::uint32_t Padding2 = 0u;
};

static_assert(std::is_standard_layout_v<ReferencePathTracerAccumulationUniformData>);
static_assert(std::is_trivially_copyable_v<ReferencePathTracerAccumulationUniformData>);
static_assert(sizeof(ReferencePathTracerAccumulationUniformData) == 16);

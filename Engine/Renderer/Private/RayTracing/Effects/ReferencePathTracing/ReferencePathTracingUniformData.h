#pragma once

#include <cstdint>
#include <type_traits>

struct ReferencePathTracingUniformData
{
	std::uint32_t SamplesPerPixel = 1u;
	std::uint32_t BounceCount = 1u;
	float NormalBias = 0.0f;
	float MaxDistance = 0.0f;
};

static_assert(std::is_standard_layout_v<ReferencePathTracingUniformData>, "ReferencePathTracingUniformData must be standard-layout");
static_assert(std::is_trivially_copyable_v<ReferencePathTracingUniformData>, "ReferencePathTracingUniformData must be trivially copyable");
static_assert(sizeof(ReferencePathTracingUniformData) == 16, "ReferencePathTracingUniformData must match the shader layout");

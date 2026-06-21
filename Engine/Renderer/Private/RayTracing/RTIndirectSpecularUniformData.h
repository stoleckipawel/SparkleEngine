#pragma once

#include <cstdint>
#include <type_traits>

struct RTIndirectSpecularUniformData
{
	std::uint32_t DebugMode = 0u;
	float NormalBias = 0.0f;
	float MaxDistance = 0.0f;
	float Padding0 = 0.0f;
};

static_assert(std::is_standard_layout_v<RTIndirectSpecularUniformData>, "RTIndirectSpecularUniformData must be standard-layout");
static_assert(std::is_trivially_copyable_v<RTIndirectSpecularUniformData>, "RTIndirectSpecularUniformData must be trivially copyable");
static_assert(sizeof(RTIndirectSpecularUniformData) == 16, "RTIndirectSpecularUniformData must match the shader layout");


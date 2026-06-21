#pragma once

#include <cstdint>
#include <type_traits>

struct RTIndirectSpecularUniformData
{
	std::uint32_t DebugMode = 0u;
	std::uint32_t HitDataAvailable = 0u;
	float NormalBias = 0.0f;
	float MaxDistance = 0.0f;
	std::uint32_t HitInstanceCount = 0u;
	std::uint32_t HitMaterialCount = 0u;
	std::uint32_t Padding0 = 0u;
	std::uint32_t Padding1 = 0u;
};

static_assert(std::is_standard_layout_v<RTIndirectSpecularUniformData>, "RTIndirectSpecularUniformData must be standard-layout");
static_assert(std::is_trivially_copyable_v<RTIndirectSpecularUniformData>, "RTIndirectSpecularUniformData must be trivially copyable");
static_assert(sizeof(RTIndirectSpecularUniformData) == 32, "RTIndirectSpecularUniformData must match the shader layout");

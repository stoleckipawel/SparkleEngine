#pragma once

#include <cstdint>
#include <type_traits>

struct IndirectSpecularUniformData
{
	std::uint32_t DebugMode = 0u;
	std::uint32_t RayTracingHitDataAvailable = 0u;
	float NormalBias = 0.0f;
	float MaxDistance = 0.0f;
	std::uint32_t RayTracingHitInstanceCount = 0u;
	std::uint32_t RayTracingHitMaterialCount = 0u;
	std::uint32_t SampleMode = 0u;
	std::uint32_t MaterialTextureTableAvailable = 0u;
	std::uint32_t MaterialTextureTableDescriptorCount = 0u;
	std::uint32_t MaterialTextureTableCapacity = 0u;
	std::uint32_t BounceCount = 1u;
	std::uint32_t Padding1 = 0u;
};

static_assert(std::is_standard_layout_v<IndirectSpecularUniformData>, "IndirectSpecularUniformData must be standard-layout");
static_assert(std::is_trivially_copyable_v<IndirectSpecularUniformData>, "IndirectSpecularUniformData must be trivially copyable");
static_assert(sizeof(IndirectSpecularUniformData) == 48, "IndirectSpecularUniformData must match the shader layout");

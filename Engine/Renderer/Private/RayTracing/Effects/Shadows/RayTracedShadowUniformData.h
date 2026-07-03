#pragma once

#include <cstdint>
#include <type_traits>

struct RayTracedShadowUniformData
{
	std::uint32_t DirectionalShadowsEnabled = 0u;
	std::uint32_t LocalLightShadowsEnabled = 0u;
	std::uint32_t Padding0 = 0u;
	std::uint32_t Padding1 = 0u;
	float NormalBias = 0.0f;
	float MaxDistance = 0.0f;
	float Padding2 = 0.0f;
	float Padding3 = 0.0f;
	std::uint32_t SceneTlasGpuAddressLow = 0u;
	std::uint32_t SceneTlasGpuAddressHigh = 0u;
	std::uint32_t RayTracingHitDataAvailable = 0u;
	std::uint32_t RayTracingHitInstanceCount = 0u;
	std::uint32_t RayTracingHitMaterialCount = 0u;
	std::uint32_t Padding4 = 0u;
	std::uint32_t Padding5 = 0u;
	std::uint32_t Padding6 = 0u;
};

static_assert(std::is_standard_layout_v<RayTracedShadowUniformData>, "RayTracedShadowUniformData must be standard-layout");
static_assert(std::is_trivially_copyable_v<RayTracedShadowUniformData>, "RayTracedShadowUniformData must be trivially copyable");
static_assert(sizeof(RayTracedShadowUniformData) == 64, "RayTracedShadowUniformData must match the shader layout");

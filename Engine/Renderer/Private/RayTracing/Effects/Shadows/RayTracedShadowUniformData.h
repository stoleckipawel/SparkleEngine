#pragma once

#include "RayTracing/Scene/RayTracingSceneTlasShaderAccessMode.h"

#include <cstdint>
#include <type_traits>

struct RayTracedShadowUniformData
{
	std::uint32_t DirectionalShadowsEnabled = 0u;
	std::uint32_t LocalLightShadowsEnabled = 0u;
	std::uint32_t DiagnosticsEnabled = 0u;
	std::uint32_t QualityMode = 0u;
	float NormalBias = 0.0f;
	float MaxDistance = 0.0f;
	float Padding0 = 0.0f;
	float Padding1 = 0.0f;
	std::uint32_t SceneTlasGpuAddressLow = 0u;
	std::uint32_t SceneTlasGpuAddressHigh = 0u;
	std::uint32_t TlasAccessMode = static_cast<std::uint32_t>(RayTracingSceneTlasShaderAccessMode::Descriptor);
	std::uint32_t Padding2 = 0u;
};

static_assert(std::is_standard_layout_v<RayTracedShadowUniformData>, "RayTracedShadowUniformData must be standard-layout");
static_assert(std::is_trivially_copyable_v<RayTracedShadowUniformData>, "RayTracedShadowUniformData must be trivially copyable");
static_assert(sizeof(RayTracedShadowUniformData) == 48, "RayTracedShadowUniformData must match the shader layout");

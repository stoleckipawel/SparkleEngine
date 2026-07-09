#pragma once

#include <cstdint>

struct RayTracingCapabilityReport;

struct RayTracedShadowSettings final
{
	static constexpr std::uint32_t RaysPerPixel = 1u;

	float NormalBias = 0.0f;
	float MaxDistance = 0.0f;
};

#pragma once

#include <cstdint>

enum class RayTracedShadowQualityMode : std::uint32_t
{
	Hard = 0,
	SoftAreaLights,
	Count
};

constexpr const char* RayTracedShadowQualityModeToString(RayTracedShadowQualityMode mode) noexcept
{
	switch (mode)
	{
		case RayTracedShadowQualityMode::Hard:
			return "Hard";
		case RayTracedShadowQualityMode::SoftAreaLights:
			return "SoftAreaLights";
		case RayTracedShadowQualityMode::Count:
		default:
			return "Unknown";
	}
}

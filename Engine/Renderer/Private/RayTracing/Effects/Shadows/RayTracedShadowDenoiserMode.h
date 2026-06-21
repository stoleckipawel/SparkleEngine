#pragma once

#include <cstdint>

enum class RayTracedShadowDenoiserMode : std::uint32_t
{
	Off = 0,
	NrdSigma,
	Count
};

constexpr const char* RayTracedShadowDenoiserModeToString(RayTracedShadowDenoiserMode mode) noexcept
{
	switch (mode)
	{
		case RayTracedShadowDenoiserMode::Off:
			return "Off";
		case RayTracedShadowDenoiserMode::NrdSigma:
			return "NrdSigma";
		case RayTracedShadowDenoiserMode::Count:
		default:
			return "Unknown";
	}
}

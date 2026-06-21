#pragma once

#include <cstdint>

enum class MaterialBindingMode : std::uint32_t
{
	RaytracingOnly = 0,
	Everything = 1,
};

constexpr const char* MaterialBindingModeToString(MaterialBindingMode mode) noexcept
{
	switch (mode)
	{
		case MaterialBindingMode::RaytracingOnly:
			return "RaytracingOnly";
		case MaterialBindingMode::Everything:
			return "Everything";
		default:
			return "Unknown";
	}
}

constexpr std::uint32_t MaterialBindingModeMask(MaterialBindingMode mode) noexcept
{
	return 1u << static_cast<std::uint32_t>(mode);
}

#pragma once

#include <cstdint>

namespace WorldCoordinates
{
	inline constexpr std::uint32_t kCoordinateContractVersion = 1u;
	inline constexpr float kMetersPerWorldUnit = 1.0f;

	inline constexpr float kRightX = 1.0f;
	inline constexpr float kRightY = 0.0f;
	inline constexpr float kRightZ = 0.0f;
	inline constexpr float kUpX = 0.0f;
	inline constexpr float kUpY = 1.0f;
	inline constexpr float kUpZ = 0.0f;
	inline constexpr float kForwardX = 0.0f;
	inline constexpr float kForwardY = 0.0f;
	inline constexpr float kForwardZ = 1.0f;
}

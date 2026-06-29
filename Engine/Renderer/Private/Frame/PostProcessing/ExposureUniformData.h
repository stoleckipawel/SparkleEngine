#pragma once

#include <cstdint>

struct ExposureUniformData final
{
	std::uint32_t ExposureMode = 1u;
	std::uint32_t ExposureHistoryValid = 0u;
	float FrameDeltaSeconds = 1.0f / 60.0f;
	float ManualExposure = 1.0f;
	float ExposureCompensation = 0.0f;
	float ExposureTargetLuminance = 0.18f;
	float ExposureMin = 0.000001f;
	float ExposureMax = 65536.0f;
	float ExposureAdaptationSpeedUp = 3.0f;
	float ExposureAdaptationSpeedDown = 1.0f;
	std::uint32_t ExposurePadding0 = 0u;
	std::uint32_t ExposurePadding1 = 0u;
};

static_assert(sizeof(ExposureUniformData) == 48);

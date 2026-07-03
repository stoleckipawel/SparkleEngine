#pragma once

#include "Upscaling/UpscalerProvider.h"

#include <cstdint>

enum class EUpscalerQualityMode : std::uint8_t
{
	NativeAA = 0,
	Quality = 1,
	Balanced = 2,
	Performance = 3,
	UltraPerformance = 4
};

struct UpscalerSettings final
{
	EUpscalerProviderKind RequestedProvider = EUpscalerProviderKind::NvidiaDlss;
	EUpscalerQualityMode QualityMode = EUpscalerQualityMode::NativeAA;
};

UpscalerSettings BuildUpscalerSettingsFromCVars() noexcept;
const char* UpscalerQualityModeToString(EUpscalerQualityMode mode) noexcept;

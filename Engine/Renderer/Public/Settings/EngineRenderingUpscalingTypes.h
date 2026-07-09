#pragma once

#include <cstdint>

enum class EUpscalerProviderKind : std::uint8_t
{
	Linear = 0,
	NvidiaDlss = 1
};

enum class EUpscalerQualityMode : std::uint8_t
{
	NativeAA = 0,
	Quality = 1,
	Balanced = 2,
	Performance = 3,
	UltraPerformance = 4
};

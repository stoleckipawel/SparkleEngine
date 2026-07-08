#pragma once

enum class EngineUpscalerProvider
{
	Linear = 0,
	NvidiaDlss = 1
};

enum class EngineUpscalerQualityMode
{
	NativeAA = 0,
	Quality = 1,
	Balanced = 2,
	Performance = 3,
	UltraPerformance = 4
};

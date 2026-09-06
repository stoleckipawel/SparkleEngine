#pragma once

#include <cstdint>

enum class EngineToneMapper : std::uint8_t
{
	Reinhard,
	AcesApprox,
	AcesFilmic,
};

enum class EngineExposureMode : std::uint8_t
{
	Manual,
	Automatic,
};

enum class EngineExposureMeteringMethod : std::uint8_t
{
	ParallelReduction,
	DownsamplePyramid,
};

enum class EngineOutputColorEncoding : std::uint8_t
{
	Automatic,
	Linear,
	Srgb,
};

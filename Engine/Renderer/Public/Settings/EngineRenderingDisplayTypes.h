#pragma once

enum class EngineToneMapper
{
	Reinhard,
	AcesApprox,
	AcesFilmic,
};

enum class EngineExposureMode
{
	Manual,
	Automatic,
};

enum class EngineExposureMeteringMethod
{
	ParallelReduction,
	DownsamplePyramid,
};

enum class EngineOutputColorEncoding
{
	Automatic,
	Linear,
	Srgb,
};

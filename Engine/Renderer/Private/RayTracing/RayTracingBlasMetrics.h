#pragma once

#include <cstdint>

struct RayTracingBlasMetrics final
{
	std::uint32_t ReferencedMeshCount = 0;
	std::uint32_t BuiltCount = 0;
	std::uint32_t ReusedCount = 0;
	double CpuMilliseconds = 0.0;
	double GpuMilliseconds = 0.0;
};

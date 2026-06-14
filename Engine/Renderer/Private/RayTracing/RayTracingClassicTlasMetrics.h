#pragma once

#include <cstdint>

struct RayTracingClassicTlasMetrics final
{
	std::uint32_t CandidateInstanceCount = 0;
	std::uint32_t InstanceCount = 0;
	std::uint32_t MissingGpuMeshCount = 0;
	std::uint32_t RejectedBlasCount = 0;
	bool Built = false;
	double CpuMilliseconds = 0.0;
	double InstancePreparationCpuMilliseconds = 0.0;
	double GpuMilliseconds = 0.0;
};

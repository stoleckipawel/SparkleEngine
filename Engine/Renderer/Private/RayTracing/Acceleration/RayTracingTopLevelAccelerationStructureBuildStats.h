#pragma once

#include <cstdint>

struct RayTracingTopLevelAccelerationStructureBuildStats final
{
	struct CandidateCounters final
	{
		std::uint32_t InstanceCount = 0;
		std::uint32_t MissingGpuMeshCount = 0;
		std::uint32_t RejectedBlasCount = 0;
	};

	struct BuildCounters final
	{
		std::uint32_t InstanceCount = 0;
		bool Built = false;
	};

	CandidateCounters Candidates;
	BuildCounters Build;
};

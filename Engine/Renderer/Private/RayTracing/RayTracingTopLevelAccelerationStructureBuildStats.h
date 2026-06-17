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

	struct PtlasPlannerCounters final
	{
		std::uint32_t TotalRenderInstanceCount = 0;
		std::uint32_t TraceableInstanceCount = 0;
		std::uint32_t StaticTraceableInstanceCount = 0;
		std::uint32_t DynamicTraceableInstanceCount = 0;
		std::uint32_t PartitionsPerAxis = 0;
		std::uint32_t PartitionCount = 0;
		std::uint32_t GridPartitionCount = 0;
		std::uint32_t DirtyTransformCount = 0;
		std::uint32_t MovedPartitionCount = 0;
		std::uint32_t GlobalPartitionEligibleCount = 0;
		std::uint32_t GlobalPartitionInstanceCount = 0;
		std::uint32_t ActivePartitionCount = 0;
		std::uint32_t MaxPartitionActivityCount = 0;
		std::uint32_t DuplicateStableIndexCount = 0;
		bool Overflow = false;
	};

	CandidateCounters Candidates;
	BuildCounters Build;
	PtlasPlannerCounters PtlasPlanner;
};

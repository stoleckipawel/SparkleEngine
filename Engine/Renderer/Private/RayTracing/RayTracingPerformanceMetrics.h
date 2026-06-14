#pragma once

#include "RHI/Public/RayTracing/RhiRayTracingDesc.h"

#include <cstdint>

struct RayTracingPerformanceMetrics final
{
	ERhiRayTracingTopLevelProvider TopLevelProvider = ERhiRayTracingTopLevelProvider::None;
	ERhiPartitionedTlasProvider PartitionedTlasProvider = ERhiPartitionedTlasProvider::None;
	bool SupportsPartitionedTlas = false;
	std::uint32_t ReferencedMeshCount = 0;
	std::uint32_t BuiltBlasCount = 0;
	std::uint32_t ReusedBlasCount = 0;
	std::uint32_t CandidateInstanceCount = 0;
	std::uint32_t TlasInstanceCount = 0;
	std::uint32_t MissingGpuMeshCount = 0;
	std::uint32_t RejectedBlasCount = 0;
	bool BuiltTlas = false;
	double ScenePrepareCpuMilliseconds = 0.0;
	double SceneBuildCpuMilliseconds = 0.0;
	double BlasCpuMilliseconds = 0.0;
	double TlasCpuMilliseconds = 0.0;
	double TlasInstancePreparationCpuMilliseconds = 0.0;
	double BlasGpuMilliseconds = 0.0;
	double ClassicTlasGpuMilliseconds = 0.0;
	double RayTracingPassGpuMilliseconds = 0.0;
};

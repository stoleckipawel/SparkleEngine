#pragma once

#include "RayTracing/RayTracingCapabilityReport.h"
#include "RayTracing/RayTracingBlasCache.h"
#include "RayTracing/RayTracingTopLevelAccelerationStructureBuildStats.h"

#include <cstdint>

class RayTracingSceneDiagnostics final
{
  public:
	RayTracingSceneDiagnostics() noexcept = default;

	void LogSceneUpdate(
	    const RayTracingCapabilityReport& capabilityReport,
	    ERhiRayTracingTopLevelProvider activeProvider,
	    const char* activeProviderReason,
	    const RayTracingBlasCache::BuildStats& blasStats,
	    const RayTracingTopLevelAccelerationStructureBuildStats& topLevelStats) noexcept;

  private:
	std::uint32_t m_lastReferencedMeshCount = 0;
	std::uint32_t m_lastBuiltBlasCount = 0;
	std::uint32_t m_lastReusedBlasCount = 0;
	std::uint32_t m_lastCandidateInstanceCount = 0;
	std::uint32_t m_lastTlasInstanceCount = 0;
	std::uint32_t m_lastMissingGpuMeshCount = 0;
	std::uint32_t m_lastRejectedBlasCount = 0;
	std::uint32_t m_lastPartitionCount = 0;
	std::uint32_t m_lastDirtyTransformCount = 0;
	std::uint32_t m_lastMovedPartitionCount = 0;
	std::uint32_t m_lastGlobalPartitionInstanceCount = 0;
	std::uint32_t m_lastDuplicateStableIndexCount = 0;
	bool m_lastPartitionOverflow = false;
	bool m_lastBuiltTlas = false;
	bool m_hasLoggedSceneSummary = false;
};

#pragma once

#include "RayTracing/RayTracingCapabilityReport.h"
#include "RayTracing/RayTracingBlasCache.h"
#include "RayTracing/RayTracingTlasBuilder.h"

#include <cstdint>

class RayTracingSceneDiagnostics final
{
  public:
	RayTracingSceneDiagnostics() noexcept = default;

	void LogSceneUpdate(
	    const RayTracingCapabilityReport& capabilityReport,
	    const RayTracingBlasCache::BuildStats& blasStats,
	    const RayTracingTlasBuilder::BuildStats& tlasStats) noexcept;

  private:
	std::uint32_t m_lastReferencedMeshCount = 0;
	std::uint32_t m_lastBuiltBlasCount = 0;
	std::uint32_t m_lastReusedBlasCount = 0;
	std::uint32_t m_lastCandidateInstanceCount = 0;
	std::uint32_t m_lastTlasInstanceCount = 0;
	std::uint32_t m_lastMissingGpuMeshCount = 0;
	std::uint32_t m_lastRejectedBlasCount = 0;
	bool m_lastBuiltTlas = false;
	bool m_hasLoggedSceneSummary = false;
};

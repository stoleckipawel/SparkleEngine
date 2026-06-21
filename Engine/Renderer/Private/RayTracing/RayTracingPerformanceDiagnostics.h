#pragma once

#include "RayTracing/RayTracingPerformanceMetrics.h"

#include "Diagnostics/FrameExecutionDiagnostics.h"

#include <cstdint>
#include <string_view>

class PassExecutionDiagnostics;

class RayTracingPerformanceDiagnostics final
{
  public:
	explicit RayTracingPerformanceDiagnostics(
	    RayTracingPerformanceMetrics& metrics,
	    PassExecutionDiagnostics* passDiagnostics = nullptr) noexcept;

	ScopedGpuEvent BeginGpuEvent(std::string_view label) noexcept;
	ScopedGpuTimer BeginGpuTimer(std::string_view label) noexcept;
	ScopedGpuScope BeginGpuScope(std::string_view label) noexcept;

	class CpuScope final
	{
	  public:
		CpuScope() noexcept = default;
		explicit CpuScope(double* target) noexcept;
		~CpuScope() noexcept;

		CpuScope(const CpuScope&) = delete;
		CpuScope& operator=(const CpuScope&) = delete;
		CpuScope(CpuScope&& other) noexcept;
		CpuScope& operator=(CpuScope&& other) noexcept;

	  private:
		void Reset() noexcept;

		double* m_target = nullptr;
		std::uint64_t m_startMicroseconds = 0;
	};

	CpuScope BeginScenePrepareCpuScope() noexcept;
	CpuScope BeginSceneBuildCpuScope() noexcept;
	CpuScope BeginBlasCpuScope() noexcept;
	CpuScope BeginTlasCpuScope() noexcept;
	CpuScope BeginTlasInstancePreparationCpuScope() noexcept;
	CpuScope BeginPartitionedTlasCpuPackScope() noexcept;

	void AddBlasGpuMilliseconds(double milliseconds) noexcept
	{
		if (m_metrics != nullptr)
		{
			m_metrics->Blas.GpuMilliseconds += milliseconds;
		}
	}

	void SetClassicTlasGpuMilliseconds(double milliseconds) noexcept
	{
		if (m_metrics != nullptr)
		{
			m_metrics->ClassicTlas.GpuMilliseconds = milliseconds;
		}
	}

	void SetRayTracingPassGpuMilliseconds(double milliseconds) noexcept
	{
		if (m_metrics != nullptr)
		{
			m_metrics->Timings.RayTracingPassGpuMilliseconds = milliseconds;
		}
	}

	static void BeginResolvedGpuTimingFrame(RayTracingPerformanceMetrics& metrics) noexcept;
	static void PublishResolvedGpuTiming(RayTracingPerformanceMetrics& metrics, const ResolvedGpuTiming& timing) noexcept;

  private:
	RayTracingPerformanceMetrics* m_metrics = nullptr;
	PassExecutionDiagnostics* m_passDiagnostics = nullptr;
};

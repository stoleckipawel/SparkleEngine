#pragma once

#include "RayTracing/RayTracingPerformanceMetrics.h"

#include "Diagnostics/FrameExecutionDiagnostics.h"

#include <cstdint>
#include <string_view>

class PassExecutionDiagnostics;

#ifndef SPARKLE_RENDERER_RAYTRACING_PERF_DIAGNOSTICS
	#if SPARKLE_BUILD_SHIPPING
		#define SPARKLE_RENDERER_RAYTRACING_PERF_DIAGNOSTICS 0
	#else
		#define SPARKLE_RENDERER_RAYTRACING_PERF_DIAGNOSTICS 1
	#endif
#endif

class RayTracingPerformanceDiagnostics final
{
  public:
	explicit RayTracingPerformanceDiagnostics(
	    RayTracingPerformanceMetrics& metrics,
	    PassExecutionDiagnostics* passDiagnostics = nullptr) noexcept;

	ScopedGpuEvent BeginGpuEvent(std::string_view label) noexcept;
	ScopedGpuTimer BeginGpuTimer(std::string_view label) noexcept;

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

	void AddBlasGpuMilliseconds(double milliseconds) noexcept;
	void SetClassicTlasGpuMilliseconds(double milliseconds) noexcept;
	void SetRayTracingPassGpuMilliseconds(double milliseconds) noexcept;

	static void BeginResolvedGpuTimingFrame(RayTracingPerformanceMetrics& metrics) noexcept;
	static void PublishResolvedGpuTiming(RayTracingPerformanceMetrics& metrics, const ResolvedGpuTiming& timing) noexcept;

  private:
	RayTracingPerformanceMetrics* m_metrics = nullptr;
	PassExecutionDiagnostics* m_passDiagnostics = nullptr;
};

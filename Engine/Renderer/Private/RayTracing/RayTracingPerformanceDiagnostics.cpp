#include "PCH.h"

#include "RayTracing/RayTracingPerformanceDiagnostics.h"

#include "Diagnostics/PassExecutionDiagnostics.h"

#include <string_view>

#if SPARKLE_RENDERER_RAYTRACING_PERF_DIAGNOSTICS
	#include <chrono>
#endif

namespace
{
#if SPARKLE_RENDERER_RAYTRACING_PERF_DIAGNOSTICS
	std::uint64_t NowMicroseconds() noexcept
	{
		return static_cast<std::uint64_t>(
		    std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::steady_clock::now().time_since_epoch()).count());
	}
#endif
}

RayTracingPerformanceDiagnostics::RayTracingPerformanceDiagnostics(
    RayTracingPerformanceMetrics& metrics,
    PassExecutionDiagnostics* passDiagnostics) noexcept :
    m_metrics(&metrics), m_passDiagnostics(passDiagnostics)
{
}

ScopedGpuEvent RayTracingPerformanceDiagnostics::BeginGpuEvent(std::string_view label) noexcept
{
#if SPARKLE_RENDERER_RAYTRACING_PERF_DIAGNOSTICS
	return m_passDiagnostics != nullptr ? m_passDiagnostics->BeginGpuEvent(label) : ScopedGpuEvent{};
#else
	(void) label;
	return {};
#endif
}

ScopedGpuTimer RayTracingPerformanceDiagnostics::BeginGpuTimer(std::string_view label) noexcept
{
#if SPARKLE_RENDERER_RAYTRACING_PERF_DIAGNOSTICS
	return m_passDiagnostics != nullptr ? m_passDiagnostics->BeginTimer(label) : ScopedGpuTimer{};
#else
	(void) label;
	return {};
#endif
}

RayTracingPerformanceDiagnostics::CpuScope::CpuScope(double* target) noexcept :
    m_target(target),
#if SPARKLE_RENDERER_RAYTRACING_PERF_DIAGNOSTICS
    m_startMicroseconds(NowMicroseconds())
#else
    m_startMicroseconds(0)
#endif
{
}

RayTracingPerformanceDiagnostics::CpuScope::~CpuScope() noexcept
{
	Reset();
}

RayTracingPerformanceDiagnostics::CpuScope::CpuScope(CpuScope&& other) noexcept :
    m_target(other.m_target), m_startMicroseconds(other.m_startMicroseconds)
{
	other.m_target = nullptr;
	other.m_startMicroseconds = 0;
}

RayTracingPerformanceDiagnostics::CpuScope& RayTracingPerformanceDiagnostics::CpuScope::operator=(CpuScope&& other) noexcept
{
	if (this != &other)
	{
		Reset();
		m_target = other.m_target;
		m_startMicroseconds = other.m_startMicroseconds;
		other.m_target = nullptr;
		other.m_startMicroseconds = 0;
	}

	return *this;
}

void RayTracingPerformanceDiagnostics::CpuScope::Reset() noexcept
{
#if SPARKLE_RENDERER_RAYTRACING_PERF_DIAGNOSTICS
	if (m_target != nullptr)
	{
		const std::uint64_t endMicroseconds = NowMicroseconds();
		if (endMicroseconds >= m_startMicroseconds)
		{
			*m_target += static_cast<double>(endMicroseconds - m_startMicroseconds) / 1000.0;
		}
	}
#endif

	m_target = nullptr;
}

RayTracingPerformanceDiagnostics::CpuScope RayTracingPerformanceDiagnostics::BeginScenePrepareCpuScope() noexcept
{
#if SPARKLE_RENDERER_RAYTRACING_PERF_DIAGNOSTICS
	return CpuScope{m_metrics != nullptr ? &m_metrics->Timings.ScenePrepareCpuMilliseconds : nullptr};
#else
	return {};
#endif
}

RayTracingPerformanceDiagnostics::CpuScope RayTracingPerformanceDiagnostics::BeginSceneBuildCpuScope() noexcept
{
#if SPARKLE_RENDERER_RAYTRACING_PERF_DIAGNOSTICS
	return CpuScope{m_metrics != nullptr ? &m_metrics->Timings.SceneBuildCpuMilliseconds : nullptr};
#else
	return {};
#endif
}

RayTracingPerformanceDiagnostics::CpuScope RayTracingPerformanceDiagnostics::BeginBlasCpuScope() noexcept
{
#if SPARKLE_RENDERER_RAYTRACING_PERF_DIAGNOSTICS
	return CpuScope{m_metrics != nullptr ? &m_metrics->Blas.CpuMilliseconds : nullptr};
#else
	return {};
#endif
}

RayTracingPerformanceDiagnostics::CpuScope RayTracingPerformanceDiagnostics::BeginTlasCpuScope() noexcept
{
#if SPARKLE_RENDERER_RAYTRACING_PERF_DIAGNOSTICS
	return CpuScope{m_metrics != nullptr ? &m_metrics->ClassicTlas.CpuMilliseconds : nullptr};
#else
	return {};
#endif
}

RayTracingPerformanceDiagnostics::CpuScope RayTracingPerformanceDiagnostics::BeginTlasInstancePreparationCpuScope() noexcept
{
#if SPARKLE_RENDERER_RAYTRACING_PERF_DIAGNOSTICS
	return CpuScope{m_metrics != nullptr ? &m_metrics->ClassicTlas.InstancePreparationCpuMilliseconds : nullptr};
#else
	return {};
#endif
}

void RayTracingPerformanceDiagnostics::AddBlasGpuMilliseconds(double milliseconds) noexcept
{
#if SPARKLE_RENDERER_RAYTRACING_PERF_DIAGNOSTICS
	if (m_metrics != nullptr)
	{
		m_metrics->Blas.GpuMilliseconds += milliseconds;
	}
#else
	(void) milliseconds;
#endif
}

void RayTracingPerformanceDiagnostics::SetClassicTlasGpuMilliseconds(double milliseconds) noexcept
{
#if SPARKLE_RENDERER_RAYTRACING_PERF_DIAGNOSTICS
	if (m_metrics != nullptr)
	{
		m_metrics->ClassicTlas.GpuMilliseconds = milliseconds;
	}
#else
	(void) milliseconds;
#endif
}

void RayTracingPerformanceDiagnostics::SetRayTracingPassGpuMilliseconds(double milliseconds) noexcept
{
#if SPARKLE_RENDERER_RAYTRACING_PERF_DIAGNOSTICS
	if (m_metrics != nullptr)
	{
		m_metrics->Timings.RayTracingPassGpuMilliseconds = milliseconds;
	}
#else
	(void) milliseconds;
#endif
}

void RayTracingPerformanceDiagnostics::BeginResolvedGpuTimingFrame(RayTracingPerformanceMetrics& metrics) noexcept
{
	metrics.Blas.GpuMilliseconds = 0.0;
	metrics.ClassicTlas.GpuMilliseconds = 0.0;
	metrics.Timings.RayTracingPassGpuMilliseconds = 0.0;
}

void RayTracingPerformanceDiagnostics::PublishResolvedGpuTiming(
    RayTracingPerformanceMetrics& metrics,
    const ResolvedGpuTiming& timing) noexcept
{
#if SPARKLE_RENDERER_RAYTRACING_PERF_DIAGNOSTICS
	const std::string_view label{timing.Label};
	if (label.find("BLAS Build") != std::string_view::npos)
	{
		metrics.Blas.GpuMilliseconds += timing.DurationMilliseconds;
		return;
	}
	if (label.find("Classic TLAS Build") != std::string_view::npos)
	{
		metrics.ClassicTlas.GpuMilliseconds = timing.DurationMilliseconds;
		return;
	}
	if (label.find("Ray Query Dispatch") != std::string_view::npos)
	{
		metrics.Timings.RayTracingPassGpuMilliseconds = timing.DurationMilliseconds;
		return;
	}
#else
	(void) metrics;
	(void) timing;
#endif
}

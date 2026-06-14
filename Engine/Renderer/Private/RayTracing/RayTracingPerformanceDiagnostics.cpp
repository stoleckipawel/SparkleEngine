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

RayTracingPerformanceDiagnostics::CpuScope::CpuScope(
    RayTracingPerformanceMetrics* metrics,
    double RayTracingPerformanceMetrics::* target) noexcept :
    m_metrics(metrics),
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
    m_metrics(other.m_metrics), m_target(other.m_target), m_startMicroseconds(other.m_startMicroseconds)
{
	other.m_metrics = nullptr;
	other.m_target = nullptr;
	other.m_startMicroseconds = 0;
}

RayTracingPerformanceDiagnostics::CpuScope& RayTracingPerformanceDiagnostics::CpuScope::operator=(CpuScope&& other) noexcept
{
	if (this != &other)
	{
		Reset();
		m_metrics = other.m_metrics;
		m_target = other.m_target;
		m_startMicroseconds = other.m_startMicroseconds;
		other.m_metrics = nullptr;
		other.m_target = nullptr;
		other.m_startMicroseconds = 0;
	}

	return *this;
}

void RayTracingPerformanceDiagnostics::CpuScope::Reset() noexcept
{
#if SPARKLE_RENDERER_RAYTRACING_PERF_DIAGNOSTICS
	if (m_metrics != nullptr && m_target != nullptr)
	{
		const std::uint64_t endMicroseconds = NowMicroseconds();
		if (endMicroseconds >= m_startMicroseconds)
		{
			m_metrics->*m_target += static_cast<double>(endMicroseconds - m_startMicroseconds) / 1000.0;
		}
	}
#endif

	m_metrics = nullptr;
	m_target = nullptr;
}

RayTracingPerformanceDiagnostics::CpuScope RayTracingPerformanceDiagnostics::BeginScenePrepareCpuScope() noexcept
{
#if SPARKLE_RENDERER_RAYTRACING_PERF_DIAGNOSTICS
	return CpuScope{m_metrics, &RayTracingPerformanceMetrics::ScenePrepareCpuMilliseconds};
#else
	return {};
#endif
}

RayTracingPerformanceDiagnostics::CpuScope RayTracingPerformanceDiagnostics::BeginSceneBuildCpuScope() noexcept
{
#if SPARKLE_RENDERER_RAYTRACING_PERF_DIAGNOSTICS
	return CpuScope{m_metrics, &RayTracingPerformanceMetrics::SceneBuildCpuMilliseconds};
#else
	return {};
#endif
}

RayTracingPerformanceDiagnostics::CpuScope RayTracingPerformanceDiagnostics::BeginBlasCpuScope() noexcept
{
#if SPARKLE_RENDERER_RAYTRACING_PERF_DIAGNOSTICS
	return CpuScope{m_metrics, &RayTracingPerformanceMetrics::BlasCpuMilliseconds};
#else
	return {};
#endif
}

RayTracingPerformanceDiagnostics::CpuScope RayTracingPerformanceDiagnostics::BeginTlasCpuScope() noexcept
{
#if SPARKLE_RENDERER_RAYTRACING_PERF_DIAGNOSTICS
	return CpuScope{m_metrics, &RayTracingPerformanceMetrics::TlasCpuMilliseconds};
#else
	return {};
#endif
}

RayTracingPerformanceDiagnostics::CpuScope RayTracingPerformanceDiagnostics::BeginTlasInstancePreparationCpuScope() noexcept
{
#if SPARKLE_RENDERER_RAYTRACING_PERF_DIAGNOSTICS
	return CpuScope{m_metrics, &RayTracingPerformanceMetrics::TlasInstancePreparationCpuMilliseconds};
#else
	return {};
#endif
}

void RayTracingPerformanceDiagnostics::AddBlasGpuMilliseconds(double milliseconds) noexcept
{
#if SPARKLE_RENDERER_RAYTRACING_PERF_DIAGNOSTICS
	if (m_metrics != nullptr)
	{
		m_metrics->BlasGpuMilliseconds += milliseconds;
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
		m_metrics->ClassicTlasGpuMilliseconds = milliseconds;
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
		m_metrics->RayTracingPassGpuMilliseconds = milliseconds;
	}
#else
	(void) milliseconds;
#endif
}

void RayTracingPerformanceDiagnostics::BeginResolvedGpuTimingFrame(RayTracingPerformanceMetrics& metrics) noexcept
{
	metrics.BlasGpuMilliseconds = 0.0;
	metrics.ClassicTlasGpuMilliseconds = 0.0;
	metrics.RayTracingPassGpuMilliseconds = 0.0;
}

void RayTracingPerformanceDiagnostics::PublishResolvedGpuTiming(
    RayTracingPerformanceMetrics& metrics,
    const ResolvedGpuTiming& timing) noexcept
{
#if SPARKLE_RENDERER_RAYTRACING_PERF_DIAGNOSTICS
	const std::string_view label{timing.Label};
	if (label.find("BLAS Build") != std::string_view::npos)
	{
		metrics.BlasGpuMilliseconds += timing.DurationMilliseconds;
		return;
	}
	if (label.find("Classic TLAS Build") != std::string_view::npos)
	{
		metrics.ClassicTlasGpuMilliseconds = timing.DurationMilliseconds;
		return;
	}
	if (label.find("Ray Query Dispatch") != std::string_view::npos)
	{
		metrics.RayTracingPassGpuMilliseconds = timing.DurationMilliseconds;
		return;
	}
#else
	(void) metrics;
	(void) timing;
#endif
}

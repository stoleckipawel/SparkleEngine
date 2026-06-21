#include "PCH.h"

#include "RayTracing/RayTracingPerformanceDiagnostics.h"

#include "Diagnostics/PassExecutionDiagnostics.h"

#include <chrono>
#include <string_view>

namespace RayTracingPerformanceDiagnosticsClock
{
	std::uint64_t NowMicroseconds() noexcept
	{
		return static_cast<std::uint64_t>(
		    std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::steady_clock::now().time_since_epoch()).count());
	}
}

RayTracingPerformanceDiagnostics::RayTracingPerformanceDiagnostics(
    RayTracingPerformanceMetrics& metrics,
    PassExecutionDiagnostics* passDiagnostics) noexcept :
    m_metrics(&metrics), m_passDiagnostics(passDiagnostics)
{
}

ScopedGpuEvent RayTracingPerformanceDiagnostics::BeginGpuEvent(std::string_view label) noexcept
{
	return m_passDiagnostics != nullptr ? m_passDiagnostics->BeginGpuEvent(label) : ScopedGpuEvent{};
}

ScopedGpuTimer RayTracingPerformanceDiagnostics::BeginGpuTimer(std::string_view label) noexcept
{
	return m_passDiagnostics != nullptr ? m_passDiagnostics->BeginTimer(label) : ScopedGpuTimer{};
}

ScopedGpuScope RayTracingPerformanceDiagnostics::BeginGpuScope(std::string_view label) noexcept
{
	return m_passDiagnostics != nullptr ? m_passDiagnostics->BeginGpuScope(label) : ScopedGpuScope{};
}

RayTracingPerformanceDiagnostics::CpuScope::CpuScope(double* target) noexcept :
    m_target(target), m_startMicroseconds(RayTracingPerformanceDiagnosticsClock::NowMicroseconds())
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
	if (m_target != nullptr)
	{
		const std::uint64_t endMicroseconds = RayTracingPerformanceDiagnosticsClock::NowMicroseconds();
		if (endMicroseconds >= m_startMicroseconds)
		{
			*m_target += static_cast<double>(endMicroseconds - m_startMicroseconds) / 1000.0;
		}
	}

	m_target = nullptr;
}

RayTracingPerformanceDiagnostics::CpuScope RayTracingPerformanceDiagnostics::BeginScenePrepareCpuScope() noexcept
{
	return CpuScope{m_metrics != nullptr ? &m_metrics->Timings.ScenePrepareCpuMilliseconds : nullptr};
}

RayTracingPerformanceDiagnostics::CpuScope RayTracingPerformanceDiagnostics::BeginSceneBuildCpuScope() noexcept
{
	return CpuScope{m_metrics != nullptr ? &m_metrics->Timings.SceneBuildCpuMilliseconds : nullptr};
}

RayTracingPerformanceDiagnostics::CpuScope RayTracingPerformanceDiagnostics::BeginBlasCpuScope() noexcept
{
	return CpuScope{m_metrics != nullptr ? &m_metrics->Blas.CpuMilliseconds : nullptr};
}

RayTracingPerformanceDiagnostics::CpuScope RayTracingPerformanceDiagnostics::BeginTlasCpuScope() noexcept
{
	return CpuScope{m_metrics != nullptr ? &m_metrics->ClassicTlas.CpuMilliseconds : nullptr};
}

RayTracingPerformanceDiagnostics::CpuScope RayTracingPerformanceDiagnostics::BeginTlasInstancePreparationCpuScope() noexcept
{
	return CpuScope{m_metrics != nullptr ? &m_metrics->ClassicTlas.InstancePreparationCpuMilliseconds : nullptr};
}

RayTracingPerformanceDiagnostics::CpuScope RayTracingPerformanceDiagnostics::BeginPartitionedTlasCpuPackScope() noexcept
{
	return CpuScope{m_metrics != nullptr ? &m_metrics->PtlasGpuUpdates.CpuPackMilliseconds : nullptr};
}

void RayTracingPerformanceDiagnostics::BeginResolvedGpuTimingFrame(RayTracingPerformanceMetrics& metrics) noexcept
{
	metrics.Blas.GpuMilliseconds = 0.0;
	metrics.ClassicTlas.GpuMilliseconds = 0.0;
	metrics.Timings.RayTracingPassGpuMilliseconds = 0.0;
	metrics.Timings.IndirectSpecularGpuMilliseconds = 0.0;
}

void RayTracingPerformanceDiagnostics::PublishResolvedGpuTiming(
    RayTracingPerformanceMetrics& metrics,
    const ResolvedGpuTiming& timing) noexcept
{
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
	if (label.find("Partitioned TLAS Build") != std::string_view::npos)
	{
		metrics.PtlasGpuUpdates.PtlasUpdateGpuMilliseconds = timing.DurationMilliseconds;
		return;
	}
	if (label.find("Ray Query Dispatch") != std::string_view::npos)
	{
		metrics.Timings.RayTracingPassGpuMilliseconds = timing.DurationMilliseconds;
		return;
	}
	if (label.find("RT Indirect Specular Ray Query") != std::string_view::npos)
	{
		metrics.Timings.IndirectSpecularGpuMilliseconds = timing.DurationMilliseconds;
		return;
	}
}

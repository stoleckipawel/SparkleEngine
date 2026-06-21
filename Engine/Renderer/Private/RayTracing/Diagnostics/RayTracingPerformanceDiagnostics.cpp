#include "PCH.h"

#include "RayTracing/Diagnostics/RayTracingPerformanceDiagnostics.h"

#include "Diagnostics/PassExecutionDiagnostics.h"

RayTracingPerformanceDiagnostics::RayTracingPerformanceDiagnostics(PassExecutionDiagnostics* passDiagnostics) noexcept :
    m_passDiagnostics(passDiagnostics)
{
}

ScopedGpuScope RayTracingPerformanceDiagnostics::BeginGpuScope(std::string_view label) noexcept
{
	return m_passDiagnostics != nullptr ? m_passDiagnostics->BeginGpuScope(label) : ScopedGpuScope{};
}


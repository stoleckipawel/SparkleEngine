#pragma once

#include "Diagnostics/FrameExecutionDiagnostics.h"

#include <string_view>

class PassExecutionDiagnostics;

class RayTracingPerformanceDiagnostics final
{
public:
	explicit RayTracingPerformanceDiagnostics(PassExecutionDiagnostics* passDiagnostics = nullptr) noexcept;

	ScopedGpuScope BeginGpuScope(std::string_view label) noexcept;

private:
	PassExecutionDiagnostics* m_passDiagnostics = nullptr;
};

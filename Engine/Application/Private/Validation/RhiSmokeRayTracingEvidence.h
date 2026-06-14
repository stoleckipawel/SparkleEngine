#pragma once

#include <memory>
#include <string_view>

namespace spdlog
{
	class logger;
}

struct RendererSmokeRayTracingDiagnostics;

namespace RhiSmokeRayTracingEvidence
{
	void Log(
	    const RendererSmokeRayTracingDiagnostics& diagnostics,
	    std::string_view evidenceLabel,
	    const std::shared_ptr<spdlog::logger>& logger) noexcept;
	bool Validate(
	    const RendererSmokeRayTracingDiagnostics& diagnostics,
	    std::string_view validationLabel,
	    const std::shared_ptr<spdlog::logger>& logger) noexcept;
}

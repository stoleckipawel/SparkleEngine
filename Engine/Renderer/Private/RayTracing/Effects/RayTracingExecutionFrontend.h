#pragma once

#include <cstdint>

struct RayTracingCapabilityReport;

enum class RayTracingExecutionFrontend : std::uint8_t
{
	None,
	Inline,
	Pipeline,
};

RayTracingExecutionFrontend ResolveRayTracingExecutionFrontend(const RayTracingCapabilityReport& capabilities) noexcept;

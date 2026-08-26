#pragma once

#include <cstdint>

enum class RayTracingExecutionFrontend : std::uint8_t
{
	None,
	Inline,
	Pipeline,
};

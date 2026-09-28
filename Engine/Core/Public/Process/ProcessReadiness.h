#pragma once

#include "Core/Public/CoreAPI.h"

#include <string_view>

namespace Process
{
	// Signals the parent-owned readiness channel when value matches the launch request.
	// Processes not started with a readiness request ignore the report.
	SPARKLE_CORE_API void SignalParentReadiness(std::string_view value) noexcept;
}

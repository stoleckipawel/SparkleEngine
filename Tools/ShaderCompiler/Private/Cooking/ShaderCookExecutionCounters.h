#pragma once

#include <cstddef>

struct ShaderCookExecutionCounters final
{
	std::size_t backendInvocationCount = 0;
	std::size_t cacheHitCount = 0;
	std::size_t cacheMissCount = 0;
	std::size_t processedNodeCount = 0;
};
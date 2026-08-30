#pragma once

#include "TaskGraph.h"

#include <cstdint>
#include <functional>

struct ParallelForPolicy final
{
	std::uint32_t GrainSize = 64;
	std::uint32_t SerialThreshold = 128;
	std::uint32_t MaximumPartitions = 64;
};

using ParallelForFunction = std::function<TaskResult(std::uint32_t begin, std::uint32_t end, TaskExecutionContext& context)>;

SPARKLE_TASKS_API TaskNodeHandle
ParallelFor(TaskGraphBuilder& graph, TaskDesc desc, std::uint32_t itemCount, ParallelForPolicy policy, ParallelForFunction function);

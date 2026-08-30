#pragma once

#include "TaskTypes.h"

#include <chrono>
#include <cstdint>

class TaskProfiler final
{
public:
	using TimePoint = std::chrono::steady_clock::time_point;

	static void RecordDependency(std::uint64_t generation, std::uint32_t prerequisite, std::uint32_t dependent) noexcept;
	static TimePoint Begin(const TaskDesc& desc, std::uint64_t generation, std::uint32_t taskIndex, std::uint32_t laneWorkerIndex) noexcept;
	static void End(
	    const TaskDesc& desc,
	    std::uint64_t generation,
	    std::uint32_t taskIndex,
	    std::uint32_t laneWorkerIndex,
	    const TaskResult& result,
	    TimePoint start) noexcept;
};

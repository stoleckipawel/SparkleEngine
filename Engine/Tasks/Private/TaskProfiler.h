#pragma once

#include "TaskTypes.h"

#include <chrono>
#include <cstdint>

namespace TaskDetail
{
	using TaskProfileTimePoint = std::chrono::steady_clock::time_point;

	void RecordTaskDependency(std::uint64_t generation, std::uint32_t prerequisite, std::uint32_t dependent) noexcept;
	TaskProfileTimePoint BeginTaskProfile(
	    const TaskDesc& desc,
	    std::uint64_t generation,
	    std::uint32_t taskIndex,
	    std::uint32_t laneWorkerIndex) noexcept;
	void EndTaskProfile(
	    const TaskDesc& desc,
	    std::uint64_t generation,
	    std::uint32_t taskIndex,
	    std::uint32_t laneWorkerIndex,
	    const TaskResult& result,
	    TaskProfileTimePoint start) noexcept;
}

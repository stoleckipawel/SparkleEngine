#include "TaskProfiler.h"

#if defined(_WIN32)
	#ifndef WIN32_LEAN_AND_MEAN
		#define WIN32_LEAN_AND_MEAN
	#endif
	#include <Windows.h>
	#include <TraceLoggingProvider.h>

TRACELOGGING_DEFINE_PROVIDER(
    g_sparkleTasksProvider,
    "SparkleTasks",
    (0x109d07d6, 0xb67d, 0x4e26, 0x9f, 0xa2, 0x47, 0x96, 0xea, 0xe8, 0x14, 0x83));
#endif

class TaskTraceProvider final
{
  public:
	static const char* LaneName(TaskLane lane) noexcept
	{
		switch (lane)
		{
			case TaskLane::FrameCritical:
				return "FrameCritical";
			case TaskLane::Background:
				return "Background";
			case TaskLane::BlockingIo:
				return "BlockingIo";
		}
		return "Invalid";
	}

	static const char* OutcomeName(TaskOutcome outcome) noexcept
	{
		switch (outcome)
		{
			case TaskOutcome::Succeeded:
				return "Succeeded";
			case TaskOutcome::Failed:
				return "Failed";
			case TaskOutcome::Cancelled:
				return "Cancelled";
		}
		return "Invalid";
	}

#if defined(_WIN32)
	struct TaskProviderRegistration final
	{
		TaskProviderRegistration() noexcept { TraceLoggingRegister(g_sparkleTasksProvider); }
		~TaskProviderRegistration() { TraceLoggingUnregister(g_sparkleTasksProvider); }
	};

	inline static TaskProviderRegistration g_taskProviderRegistration;
#endif
};

void TaskProfiler::RecordDependency(
    std::uint64_t generation,
    std::uint32_t prerequisite,
    std::uint32_t dependent) noexcept
{
#if defined(_WIN32)
	if (!TraceLoggingProviderEnabled(g_sparkleTasksProvider, 0, 0))
	{
		return;
	}
	TraceLoggingWrite(
	    g_sparkleTasksProvider,
	    "TaskDependency",
	    TraceLoggingUInt64(generation, "Run"),
	    TraceLoggingUInt32(prerequisite, "Prerequisite"),
	    TraceLoggingUInt32(dependent, "Dependent"));
#else
	(void)generation;
	(void)prerequisite;
	(void)dependent;
#endif
}

TaskProfiler::TimePoint TaskProfiler::Begin(
    const TaskDesc& desc,
    std::uint64_t generation,
    std::uint32_t taskIndex,
    std::uint32_t laneWorkerIndex) noexcept
{
#if defined(_WIN32)
	if (!TraceLoggingProviderEnabled(g_sparkleTasksProvider, 0, 0))
	{
		return {};
	}
	const TimePoint start = std::chrono::steady_clock::now();
	TraceLoggingWrite(
	    g_sparkleTasksProvider,
	    "TaskBegin",
	    TraceLoggingString(desc.Name.Get().data(), "Name"),
	    TraceLoggingString(TaskTraceProvider::LaneName(desc.Lane), "Lane"),
	    TraceLoggingUInt64(generation, "Run"),
	    TraceLoggingUInt32(taskIndex, "Task"),
	    TraceLoggingUInt32(laneWorkerIndex, "Worker"));
#else
	(void)desc;
	(void)generation;
	(void)taskIndex;
	(void)laneWorkerIndex;
	const TimePoint start{};
#endif
	return start;
}

void TaskProfiler::End(
    const TaskDesc& desc,
    std::uint64_t generation,
    std::uint32_t taskIndex,
    std::uint32_t laneWorkerIndex,
    const TaskResult& result,
    TimePoint start) noexcept
{
	if (start == TimePoint{})
	{
		return;
	}
	const auto duration = std::chrono::duration_cast<std::chrono::nanoseconds>(
	                          std::chrono::steady_clock::now() - start)
	                          .count();
#if defined(_WIN32)
	TraceLoggingWrite(
	    g_sparkleTasksProvider,
	    "TaskEnd",
	    TraceLoggingString(desc.Name.Get().data(), "Name"),
	    TraceLoggingString(TaskTraceProvider::LaneName(desc.Lane), "Lane"),
	    TraceLoggingUInt64(generation, "Run"),
	    TraceLoggingUInt32(taskIndex, "Task"),
	    TraceLoggingUInt32(laneWorkerIndex, "Worker"),
	    TraceLoggingInt64(duration, "DurationNs"),
	    TraceLoggingString(TaskTraceProvider::OutcomeName(result.GetOutcome()), "Status"));
#else
	(void)desc;
	(void)generation;
	(void)taskIndex;
	(void)laneWorkerIndex;
	(void)result;
	(void)duration;
#endif
}

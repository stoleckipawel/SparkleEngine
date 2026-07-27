#include "PCH.h"

#include "Concurrency/TaskRuntimeCVars.h"

#include "Core/Public/Console/CVar.h"

class TaskRuntimeCVarsState final
{
  public:
	inline static ConsoleVariable<std::uint32_t> g_frameCriticalWorkerCount{
	    "task.FrameCriticalWorkerCount", 1, "Frame-critical task workers. Sweep explicitly before changing the product default."};
	inline static ConsoleVariable<std::uint32_t> g_backgroundWorkerCount{
	    "task.BackgroundWorkerCount", 1, "Background task workers. Sweep independently from frame-critical work."};
	inline static ConsoleVariable<std::uint32_t> g_blockingIoWorkerCount{
	    "task.BlockingIoWorkerCount", 1, "Blocking-I/O task workers. Keep bounded independently from CPU task lanes."};
	inline static ConsoleVariable<bool> g_serialExecution{
	    "task.SerialExecution", false, "Run SparkleTasks on the deterministic caller-thread reference executor."};
};

namespace TaskRuntimeCVars
{
	void Register() noexcept {}
	std::uint32_t ResolveFrameCriticalWorkerCount() noexcept
	{
		return TaskRuntimeCVarsState::g_frameCriticalWorkerCount.Get();
	}

	std::uint32_t ResolveBackgroundWorkerCount() noexcept
	{
		return TaskRuntimeCVarsState::g_backgroundWorkerCount.Get();
	}

	std::uint32_t ResolveBlockingIoWorkerCount() noexcept
	{
		return TaskRuntimeCVarsState::g_blockingIoWorkerCount.Get();
	}

	bool UseSerialExecution() noexcept { return TaskRuntimeCVarsState::g_serialExecution.Get(); }
}
